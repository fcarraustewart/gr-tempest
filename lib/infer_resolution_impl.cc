/* -*- c++ -*- */
/*
 * Copyright 2020
 *   Federico "Larroca" La Rocca <flarroca@fing.edu.uy>
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "infer_resolution_impl.h"
#include <thread>
#include <volk/volk.h>
#include <math.h>

#define MIN_FRAMERATE (55)
#define MIN_HEIGHT (590)
#define MAX_FRAMERATE (85)
#define MAX_HEIGHT (1500)
#define lowpasscoeff 0.1
#define MAX_PERIOD 0.0000284

namespace gr {
  namespace tempest {

    infer_resolution::sptr
    infer_resolution::make(int sample_rate, int size, int refresh_rate, int Vvisible, int Hvisible, bool automatic_mode)
    {
      return gnuradio::get_initial_sptr
        (new infer_resolution_impl(sample_rate, size, refresh_rate, Vvisible, Hvisible, automatic_mode));
    }


    /*
     * The private constructor
     */
    infer_resolution_impl::infer_resolution_impl(int sample_rate, int size, int refresh_rate, int Vvisible, int Hvisible, bool automatic_mode)
      : gr::block("infer_resolution",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(1, 1, sizeof(float)))
    {
    //Received parameters
      d_sample_rate = sample_rate;
      d_fft_size = size;
      
      d_refresh_rate = refresh_rate;
      d_Vvisible = Vvisible;
      d_Hvisible = Hvisible;
      d_automatic_mode = automatic_mode;

      //Search values
      //d_search_skip = 830000;
      d_search_skip = d_sample_rate/(d_refresh_rate+0.2);
      //d_search_skip = d_sample_rate/(MAX_FRAMERATE);
      d_search_margin = 10000;
      
      //d_search_margin = (d_sample_rate/(MIN_FRAMERATE))-((d_sample_rate)/(MAX_FRAMERATE));
      d_vtotal_est = 0;
      
      //Parameters to publish
      d_refresh_rate = 0;
      /*d_Hvisible = 0;
      d_Vvisible = 0;*/
      d_Hblank = 0;
      d_Vblank = 0;

      //Counters
      d_work_counter = 0;
      
      //PMT ports
      message_port_register_out(pmt::mp("refresh_rate"));
      message_port_register_out(pmt::mp("Vvisible"));
      message_port_register_out(pmt::mp("Vblank"));
      message_port_register_out(pmt::mp("Hvisible"));
      message_port_register_out(pmt::mp("Hblank"));

      set_history(d_fft_size);
    }

    /*
     * Our virtual destructor.
     */
    infer_resolution_impl::~infer_resolution_impl()
    {
    }

    void
    infer_resolution_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
       ninput_items_required[0] = noutput_items;
    }

    void
    infer_resolution_impl::publish_messages()
    {
      message_port_pub(
                        pmt::mp("refresh_rate"), 
                        pmt::cons(
                          pmt::mp("refresh_rate"), 
                          pmt::from_long(d_refresh_rate)
                        )
                      );             
        
      message_port_pub(
                        pmt::mp("Vvisible"), 
                        pmt::cons(
                          pmt::mp("Vvisible"), 
                          pmt::from_long(d_Vvisible)
                        )
                      );             
      message_port_pub(
                        pmt::mp("Vblank"), 
                        pmt::cons(
                          pmt::mp("Vblank"), 
                          pmt::from_long(d_Vblank)
                        )
                      );
      message_port_pub(
                        pmt::mp("Hvisible"), 
                        pmt::cons(
                          pmt::mp("Hvisible"), 
                          pmt::from_long(d_Hvisible)
                        )
                      );     
      message_port_pub(
                        pmt::mp("Hblank"), 
                        pmt::cons(
                          pmt::mp("Hblank"), 
                          pmt::from_long(d_Hblank)
                        )
                      ); 
    }
    int
    infer_resolution_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const float  *in = (const float  *) input_items[0];
      float  *out = (float  *) output_items[0];

      // Do <+signal processing+>
      
      d_work_counter++;                                                           /* Work iteration counter */
      

      /////////////////////////////
      //   REFRESH RATE SEARCH   //
      /////////////////////////////

      uint32_t peak_index = 0, yt_index = 0, yt_aux = 0;

      volk_32f_index_max_32u(&peak_index, &in[d_search_skip], d_search_margin);   /* 'descartados' se elige para que de cerca del pico conocido */

      peak_index += d_search_skip;                                                /* Offset por indice relativo en volk */

      double fv = (double)d_sample_rate/(double)peak_index;

      d_refresh_rate = (long)fv;                        /* Intentar que varíe menos que fv */
   

      /////////////////////////////
      //     HEIGHT SEARCH       //
      /////////////////////////////

      int yt_largo = (int)d_sample_rate*MAX_PERIOD;                               /* Elegido para asegurar que encuentre pico en cualquier res */

      volk_32f_index_max_32u(&yt_index, &in[peak_index+5], yt_largo);             /* Arranca en +5 para no contar el mismo pico */

      //volk_32f_index_max_32u(&yt_index, &in[yt_aux+5], yt_largo);

      double yt = (double)d_sample_rate / (double)((yt_index+5)*fv);              /* El +5 compensa lo que se movio por el volk */

      //d_vtotal_est = ((int) round(yt * lowpasscoeff + (1.0 - lowpasscoeff) * (d_vtotal_est)));
      d_vtotal_est = round(yt);


      /////////////////////////////
      //    UPDATE RESULTS       //
      /////////////////////////////

      if(d_work_counter >= 500)
      {
        search_table(d_refresh_rate);	//cambie fv por refresh_rate
        publish_messages();

        printf("Refresh Rate \t %f \t Hz \t \t Vtotal_inst \t %f \t Px \t\t Vtotal \t %d \t Px \t\t Vvisible \t %ld \t Px \t\t Hvisible \t %ld \t Px \t Hblank \t %ld \r \n ", fv, yt,d_vtotal_est,d_Vvisible,d_Hvisible, d_Hblank);

        d_work_counter = 0;
       } 
      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (noutput_items);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }
    void
    infer_resolution_impl::search_table(double fv_estimated)
      {
        if (fv_estimated<58)
        {
          d_refresh_rate=56;
          if (d_vtotal_est<700 && d_vtotal_est>450)	//not necessary, discard big estimated error
          {
          
            d_Vvisible=600;
            d_Vblank=25;
            d_Hvisible=800;
            d_Hblank=224;
          }
        }
        
        
        if (fv_estimated<67.5 && fv_estimated>58)
        {
          d_refresh_rate=60;
          if(d_vtotal_est<689){
            d_Vvisible=600;
            d_Vblank=28;
            d_Hvisible=800;
            d_Hblank=256;
          }
          
          if(d_vtotal_est<770 && d_vtotal_est>689){
            d_Vvisible=720;		//El blanking no coincide con lo que dice el monitor
            d_Vblank=30;
            d_Hvisible=1280;
            d_Hblank=370;
          }
          if(d_vtotal_est<792.5 &&  d_vtotal_est>770){
          //reduced blanking
            d_Vvisible=768;
            d_Vblank=22;
            d_Hvisible=1280;
            d_Hblank=160;
          }
          /*if(d_vtotal_est<796.5 &&  d_vtotal_est>792.5){
            d_Hvisible=1360;
            d_Hblank=432;  
            d_Vvisible=768;
            d_Vblank=27;                
          }
          if(d_vtotal_est<799 &&  d_vtotal_est>796.5){
            d_Hvisible=1366;
            d_Hblank=426;  
            d_Vvisible=768;
            d_Vblank=30;            
          }
          if(d_vtotal_est<803 &&  d_vtotal_est>799){
          //reduced blanking
            d_Hvisible=1366;
            d_Hblank=134;  
            d_Vvisible=768;
            d_Vblank=32;            
          }*/
          if(d_vtotal_est<869.5 &&  d_vtotal_est>792.5){
          //ESTA ES UNA DE LAS GRABACIONES Y ESTA BIEN LOS DATOS
            d_Hvisible=1024;
            d_Hblank=320;  
            d_Vvisible=768;
            d_Vblank=38;            
          }
          /*if(d_vtotal_est<869.5 &&  d_vtotal_est>809.5){
          //reduced blanking
            d_Hvisible=1360;
            d_Hblank=160;  
            d_Vvisible=768;
            d_Vblank=45;            
          }*/
          if(d_vtotal_est<930 &&  d_vtotal_est>869.5){
          //reduced blanking
            d_Hvisible=1440;
            d_Hblank=160;  
            d_Vvisible=900;
            d_Vblank=26;            
          }
          if(d_vtotal_est<1000 &&  d_vtotal_est>930){
            d_Hvisible=1440;
            d_Hblank=464;  
            d_Vvisible=900;
            d_Vblank=34;            
          }
          if(d_vtotal_est<1095.5 &&  d_vtotal_est>1000){
            d_Hvisible=1280;
            d_Hblank=408;  
            d_Vvisible=1024;
            d_Vblank=42;            
          }
          if(d_vtotal_est>1095.5){
            d_Hvisible=1920;
            d_Hblank=280;  
            d_Vvisible=1080;
            d_Vblank=45;            
          }
        } 
        if (fv_estimated>67.5 && fv_estimated<72.5){
            /*add from xrandr*/
            d_refresh_rate=70;
            d_Hvisible=720;
            d_Hblank=180;  
            d_Vvisible=400;
            d_Vblank=49; 
         
          /* no esta en las opciones del monitor
          d_refresh_rate=70;
          d_Hvisible=1024;
          d_Hblank=304;  
          d_Vvisible=768;
          d_Vblank=38;   */         
        }
        
        
        if(fv_estimated>72.5 && fv_estimated<80){
          d_refresh_rate=75;
          if(d_vtotal_est<562.5){
          /* add from xrandr*/
            d_Hvisible=640;
            d_Hblank=200;  
            d_Vvisible=480;
            d_Vblank=20; 
          if(d_vtotal_est<712.5 && d_vtotal_est>562.5){
          /*check*/
            d_Hvisible=800;
            d_Hblank=256;  
            d_Vvisible=600;
            d_Vblank=25;  
          }
          if(d_vtotal_est<850 && d_vtotal_est>712.5){
            /*check*/
            d_Hvisible=1024;
            d_Hblank=288;  
            d_Vvisible=768;
            d_Vblank=32;  
          }
          /*if(d_vtotal_est<873.5 &&  d_vtotal_est>802.5){
            d_Hvisible=1280;
            d_Hblank=416;  
            d_Vvisible=768;
            d_Vblank=37;  */
          }
          if(d_vtotal_est<983 &&  d_vtotal_est>850){
            d_Hvisible=1152;
            d_Hblank=448;  
            d_Vvisible=864;
            d_Vblank=36;  
          }
          
          //if(d_vtotal_est<1004 &&  d_vtotal_est>802.5){
            /*d_Hvisible=1440;
            d_Hblank=496;  
            d_Vvisible=900;
            d_Vblank=42;  
          }*/
          if(d_vtotal_est>983){
            /*check*/
            d_Hvisible=1280;
            d_Hblank=408;  
            d_Vvisible=1024;
            d_Vblank=42;  
          }
        }
        if(fv_estimated>80){
          d_refresh_rate=85;
          if(d_vtotal_est<719.5){
            d_Hvisible=800;	
            d_Hblank=248;  
            d_Vvisible=600;
            d_Vblank=31;  
          }
          if(d_vtotal_est<808.5 && d_vtotal_est>719.5){
            d_Hvisible=1024;
            d_Hblank=352;  
            d_Vvisible=768;
            d_Vblank=40;  
          }
          if(d_vtotal_est<878.5 &&  d_vtotal_est>808.5){
            d_Hvisible=1280;
            d_Hblank=432;  
            d_Vvisible=768;
            d_Vblank=41;  
          }
          if(d_vtotal_est<1010 &&  d_vtotal_est>8878.5){
            d_Hvisible=1440;
            d_Hblank=512;  
            d_Vvisible=900;
            d_Vblank=48;  
          }
          if(d_vtotal_est>1010){
            d_Hvisible=1280;
            d_Hblank=448;  
            d_Vvisible=1024;
            d_Vblank=48;  
          }
        }
    }
  } /* namespace tempest */
} /* namespace gr */

