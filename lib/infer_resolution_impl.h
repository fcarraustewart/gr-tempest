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

#ifndef INCLUDED_TEMPEST_INFER_RESOLUTION_IMPL_H
#define INCLUDED_TEMPEST_INFER_RESOLUTION_IMPL_H

#include <tempest/infer_resolution.h>

#include <gnuradio/fft/fft.h>
#include <gnuradio/api.h>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <stdio.h>
#include <math.h>
#include <random>
#include <thread>
#include <chrono>
#include <volk/volk.h>

#define FRAMES_TO_PROCESS 3
#define MAX_H_TOTAL 2200
#define MAX_V_TOTAL 1125

namespace gr {
  namespace tempest {

    namespace marinov {     
          uint32_t fft_get_real_size(uint32_t size) 
          {
              uint32_t m =0;
              while ((size /= 2) != 0)
                      m++;

              return 1 << m;
          }
    }

    class infer_resolution_impl : public infer_resolution
    {
     private:
      gr_complex* d_buffer1_;
      gr_complex* d_buffer2_;

      uint32_t d_fix_fft_size;
      int d_update_timer_seconds;
      
      gr::fft::fft_complex* d_fft;
      gr::fft::fft_complex* d_ifft;

      uint32_t d_calculated_fft_size = 0;
      uint8_t d_end = 0;
      void send_pmt_messages(double refresh_rate, long v_visible, long v_blank, long h_visible, long h_blank);
     
     public:
      infer_resolution_impl(int update_timer_seconds);
      ~infer_resolution_impl();

      // Where all the action really happens
      int work(
              int noutput_items,
              gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items
      );
    };

  } // namespace tempest
} // namespace gr

#endif /* INCLUDED_TEMPEST_INFER_RESOLUTION_IMPL_H */

