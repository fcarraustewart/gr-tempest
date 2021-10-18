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
 * \file infer_screen_resolution_impl.h
 * 
 * \brief Block that searches for the autocorrelation peaks to
 * infer the resolution information from the screen signal.
 * The user is required to input the peak time to confirm
 * refresh rate, so the screen sizes can be easily found.
 *
 * gr-tempest
 *
 * \date October 6, 2021
 * \author  Pablo Bertrand   <pablo.bertrand@fing.edu.uy>
 * \author  Felipe Carrau    <felipe.carrau@fing.edu.uy>
 * \author  Victoria Severi  <maria.severi@fing.edu.uy>
 */

/**********************************************************
 * Constant and macro definitions
 **********************************************************/

#ifndef INCLUDED_TEMPEST_INFER_SCREEN_RESOLUTION_IMPL_H
#define INCLUDED_TEMPEST_INFER_SCREEN_RESOLUTION_IMPL_H

#define lowpasscoeff 0.1
#define MAX_PERIOD 0.0000284

/**********************************************************
 * Include statements
 **********************************************************/

#include <tempest/infer_screen_resolution.h>

namespace gr {
  namespace tempest {

    class infer_screen_resolution_impl : public infer_screen_resolution
    {
     private:
      /**********************************************************
       * Data declarations
       **********************************************************/

      //Received parameters
      float d_sample_rate;
      int d_fft_size;
      float d_refresh_rate;

      //Search values
      uint32_t d_search_skip;
      uint32_t d_search_margin;
      uint32_t d_vtotal_est;
      float d_refresh_rate_est;
      bool d_flag;

      //Resolution results
      int d_Hsize;
      int d_Vsize;
      int d_Vvisible;
      int d_Hvisible;

      //Counters
      uint32_t d_work_counter;

      /**********************************************************
       * Private function prototypes
       **********************************************************/
      /*!
        * \brief Function that uses a lookup table to find the
        * appropriate resolution based on the refresh rate and
        * the estimated vertical total.
        * 
        * \param double fv_estimated: estimated refresh rate.
        */
      void search_table(float fv_estimated);
      //--------------------------------------------------------- 
     
     public:
      infer_screen_resolution_impl(int sample_rate , int fft_size, float refresh_rate);
      ~infer_screen_resolution_impl();

      /**********************************************************
       * Public function prototypes
       **********************************************************/
      /*!
        * \brief Callback function to set the search values when
        * parameters are modified in runtime.
        * 
        * \param float refresh_rate: new value for the refresh
        * rate parameter.
        */
      void set_refresh_rate(float refresh_rate);
      //---------------------------------------------------------      
      /*!
        * \brief Used to establish the amount of samples required
        * for a full work iteration.
        */
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);
      //---------------------------------------------------------
      /*!
        * \brief Searches first for the first peak, confirming the
        * refresh rate input data, and then the second peak, to
        * define the line time and thus find the vertical resolution.
        * With those two values it searches the lookup table to 
        * complete the resolution information. Results are printed
        * in terminal once every few seconds.
        */
      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
      //---------------------------------------------------------
    };

  } // namespace tempest
} // namespace gr

#endif /* INCLUDED_TEMPEST_INFER_SCREEN_RESOLUTION_IMPL_H */

