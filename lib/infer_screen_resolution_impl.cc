/* -*- c++ -*- */
/*!
 * Copyright 2021
 *    Pablo Bertrand    <pablo.bertrand@fing.edu.uy>
 *    Felipe Carrau     <felipe.carrau@fing.edu.uy>
 *    Victoria Severi   <maria.severi@fing.edu.uy>
 *
 *   Instituto de Ingenieria Electrica, Facultad de Ingenieria,
 *   Universidad de la Republica, Uruguay.
 *  
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *  
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 *
 * \file infer_screen_resolution_impl.cc
 * 
 * gr-tempest
 *
 * \date October 6, 2021
 * \author  Pablo Bertrand   <pablo.bertrand@fing.edu.uy>
 * \author  Felipe Carrau    <felipe.carrau@fing.edu.uy>
 * \author  Victoria Severi  <maria.severi@fing.edu.uy>
 */

/**********************************************************
 * Include statements
 **********************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "infer_screen_resolution_impl.h"
#include <thread>
#include <volk/volk.h>
#include <math.h>

/**********************************************************
 * Constant and macro definitions
 **********************************************************/

#define MIN_FRAMERATE (55)
#define MIN_HEIGHT (590)
#define MAX_FRAMERATE (85)
#define MAX_HEIGHT (1500)
#define lowpasscoeff 0.1
#define MAX_PERIOD 0.0000284

namespace gr {
  namespace tempest {

    infer_screen_resolution::sptr
    infer_screen_resolution::make(int sample_rate , int fft_size, float refresh_rate)
    {
      return gnuradio::get_initial_sptr
        (new infer_screen_resolution_impl(sample_rate, fft_size, refresh_rate));
    }

    /**********************************************************
     * Function bodies
     **********************************************************/
    /*
     * The private constructor
     */
    infer_screen_resolution_impl::infer_screen_resolution_impl(int sample_rate , int fft_size, float refresh_rate)
      : gr::block("infer_screen_resolution",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(1, 1, sizeof(float)))
    {
      //Received parameters
      d_sample_rate = sample_rate;
      d_fft_size = fft_size;
      set_refresh_rate(refresh_rate);

      //Search values
      d_search_margin = 10000;
      d_flag = false;
      
      //Resolution results
      d_Hvisible = 0;
      d_Vvisible = 0;
      d_Hsize = 0;
      d_Vsize = 0;

      //Counters
      d_work_counter = 0;

      set_history(d_fft_size);

      printf("[TEMPEST] Welcome to the Resolution Inference block. Please refer 
        to the graphic on the Time Sink to find the peak and enter its approximate 
        time value in microseconds.");
    }
    
    //---------------------------------------------------------
    /*
     * Our virtual destructor.
     */
    infer_screen_resolution_impl::~infer_screen_resolution_impl()
    {
    }

    //---------------------------------------------------------

    void infer_screen_resolution_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      ninput_items_required[0] = noutput_items;
    }

    //---------------------------------------------------------

    void infer_screen_resolution_impl::set_refresh_rate(int refresh_rate)
    {
      //If the refresh rate changed, parameters are reset with callback
      d_refresh_rate = refresh_rate;
      d_search_skip = d_sample_rate/(d_refresh_rate+0.2);
      d_refresh_rate_est=refresh_rate;
	    printf("[TEMPEST] Setting refresh to %i in infer block.\n", refresh_rate);
	  }

    //---------------------------------------------------------

    int infer_screen_resolution_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const float  *in = (const float  *) input_items[0];
      float  *out = (float  *) output_items[0];

      //Work iteration counter
      d_work_counter++;                                                           
      
      /////////////////////////////
      //   REFRESH RATE SEARCH   //
      /////////////////////////////

      uint32_t peak_index = 0, yt_index = 0, yt_aux = 0;

      volk_32f_index_max_32u(&peak_index, &in[d_search_skip], d_search_margin); 

      // Offset because of relative volk index:
      peak_index += d_search_skip;                                                

      memcpy(out, in, noutput_items*sizeof(float));

      // Add Tag to the peak:
      add_item_tag(0, nitems_written(0)+peak_index, pmt::mp("peak"), pmt::PMT_T);

      double fv = (double)d_sample_rate/(double)peak_index;

      // Lower the variation of the received refresh rate:
   	  d_refresh_rate_est = ((long) round(fv * lowpasscoeff + (1.0 - lowpasscoeff) * (d_refresh_rate_est)));
   	
      /////////////////////////////
      //     HEIGHT SEARCH       //
      /////////////////////////////

      int yt_largo = (int)d_sample_rate*(MAX_PERIOD);

      volk_32f_index_max_32u(&yt_index, &in[peak_index+5], yt_largo);
      // The peak search begins a few samples later to avoid repeating the previous result

      double yt = (double)d_sample_rate / (double)((yt_index+5)*fv);
      // The same sample movement is compensated

      if (d_flag)  
      {
      	if (yt < 1225 && yt > 350)
      	{
      		d_vtotal_est = ((int) round(yt * lowpasscoeff + (1.0 - lowpasscoeff) * (d_vtotal_est)));
      	}     
      }
      else 
      {
      	if (yt < 1225 && yt > 350)
      	{
      		d_vtotal_est = yt;
      		d_flag = true;

      	}
      }

      /////////////////////////////
      //    UPDATE RESULTS       //
      /////////////////////////////

      if(d_work_counter >= 500)
      // Results are printed every 500 iterations to maintain information updated
      // withoud flooding the GRC terminal
      {
        search_table(d_refresh_rate_est);	
        publish_messages();

        printf(" Hdisplay \t %ld \t Px \t\t Vdisplay \t %ld \t Px \t\t Hsize \t %ld \t Px \t\t Vsize \t %ld \t Px \t\t Refresh Rate \t %ld \t Hz \t \n ", d_Hvisible,d_Vvisible,d_Hsize,d_Vsize,d_refresh_rate);

        d_work_counter = 0;
      } 

      consume_each (noutput_items);
      return noutput_items;
    }
    
    //---------------------------------------------------------

    void infer_screen_resolution_impl::search_table(double fv_estimated)
      {
      if (fv_estimated<65)
        {
          d_refresh_rate=60;
          if (d_vtotal_est<576.5)
          {
            d_Vvisible=480;
            d_Vsize=525;
            d_Hvisible=640;
            d_Hsize=800;
                      }
         if (d_vtotal_est>576.5 && d_vtotal_est<687)
          { 
            //800x600
            d_Hvisible=800;
            d_Hsize=1056;
            d_Vvisible=600;
            d_Vsize=628;
          }
           if (d_vtotal_est>687 && d_vtotal_est<776)
          { 
            //1280x720
            d_Hvisible=1280;
            d_Hsize=1664;
            d_Vvisible=720;
            d_Vsize=746;
          }
           /*if (d_vtotal_est>746 && d_vtotal_est<746)
          { 
            //1152x720
            d_Hvisible=1152;
            d_Hsize=1504;
            d_Vvisible=720;
            d_Vsize=746;
          }*/
           if (d_vtotal_est>776 && d_vtotal_est<869)
          { 
            //1024x768
            d_Hvisible=1024;
            d_Hsize=1344;
            d_Vvisible=768;
            d_Vsize=806;
          }
           if (d_vtotal_est>869 && d_vtotal_est<966)
          { 
            //1600x900
            d_Hvisible=1600;
            d_Hsize=2128;
            d_Vvisible=900;
            d_Vsize=932;
          }
           if (d_vtotal_est>966 && d_vtotal_est<1033)
          { 
            //1280x960
            d_Hvisible=1280;
            d_Hsize=1800;
            d_Vvisible=960;
            d_Vsize=1000;
          }
           if (d_vtotal_est>1033 && d_vtotal_est<1077.5)
          { 
            //1280x1024
            d_Hvisible=1280;
            d_Hsize=1688;
            d_Vvisible=1024;
            d_Vsize=1066;
          }
           if (d_vtotal_est>1077.5 && d_vtotal_est<1107)
          { 
            //1680x1050
            d_Hvisible=1680;
            d_Hsize=2240;
            d_Vvisible=1050;
            d_Vsize=1089;
          }
           if (d_vtotal_est>1107 && d_vtotal_est<1300)
          { 
            //1920x1080
            d_Hvisible=1920;
            d_Hsize=2200;
            d_Vvisible=1080;
            d_Vsize=1125;
          }
        }
      if (fv_estimated>65 && fv_estimated<72.5)
        {
          d_refresh_rate=(int)70.1;
          if (d_vtotal_est>400 && d_vtotal_est<500)
          {
            d_Hvisible=720;
            d_Hsize=900;
            d_Vvisible=400;
            d_Vsize=449;
          }
        }
       if (fv_estimated>72.5 && fv_estimated<80)
        {
          d_refresh_rate=75;
          if (d_vtotal_est<562.5)	
          {
            //640x480
            d_Hvisible=640;
            d_Hsize=840;
            d_Vvisible=480;
            d_Vsize=500;
          }
          if (d_vtotal_est>562.5 && d_vtotal_est<646)
          {
            //800x600
            d_Hvisible=800;
            d_Hsize=1056;
            d_Vvisible=600;
            d_Vsize=625;
          }
          if (d_vtotal_est>646 && d_vtotal_est<733.5)
          {
            //832x624
            d_refresh_rate=(int)74.6;
            d_Hvisible=832;
            d_Hsize=1152;
            d_Vvisible=624;
            d_Vsize=667;
          }
          if (d_vtotal_est>733.5 && d_vtotal_est<850)
          {
            //1024x768
            d_refresh_rate=(int)75.1;
            d_Hvisible=1024;
            d_Hsize=1312;
            d_Vvisible=768;
            d_Vsize=800;
          }
          if (d_vtotal_est>850 && d_vtotal_est<983)
          {
            //1152x864
            d_Hvisible=1152;
            d_Hsize=1600;
            d_Vvisible=864;
            d_Vsize=900;
          }
          if (d_vtotal_est>983 && d_vtotal_est<1250)
          {
            //1280x1024
            d_Hvisible=1280;
            d_Hsize=1688;
            d_Vvisible=1024;
            d_Vsize=1066;
          }
        }
      }


  } /* namespace tempest */
} /* namespace gr */

