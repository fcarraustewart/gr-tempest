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

namespace gr {
  namespace tempest {

    infer_resolution::sptr
    infer_resolution::make(int update_timer_seconds)
    {
      return gnuradio::get_initial_sptr
        (new infer_resolution_impl(update_timer_seconds));
    }


    /*
     * The private constructor
     */
    infer_resolution_impl::infer_resolution_impl(int update_timer_seconds)
      : gr::sync_block("infer_resolution",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(0, 0, 0))
    {
    // Voy a tener que hacer un set history del tama;o de 2 frames. Pero no se cuanto tiene una frame
    // Para eso esta el alto minimo y el alto maximo... Tambien con un ancho minimo y un ancho maximo?
      set_history(FRAMES_TO_PROCESS*MAX_H_TOTAL*MAX_V_TOTAL);  
    //PMT ports

      message_port_register_out(pmt::mp("refresh_rate"));
      message_port_register_out(pmt::mp("Vvisible"));
      message_port_register_out(pmt::mp("Vblank"));
      message_port_register_out(pmt::mp("Hvisible"));
      message_port_register_out(pmt::mp("Hblank"));

    // Member variables

      d_buffer1_ = (gr_complex*)volk_malloc(MAX_V_TOTAL*MAX_H_TOTAL,volk_get_alignment() / sizeof(gr_complex));
      d_buffer2_ = (gr_complex*)volk_malloc(MAX_V_TOTAL*MAX_H_TOTAL,volk_get_alignment() / sizeof(gr_complex));

      d_fix_fft_size = 128;

      d_fft= new gr::fft::fft_complex(   d_fix_fft_size, true, 1   );
      d_ifft= new gr::fft::fft_complex(   d_fix_fft_size, false, 1   );

      d_update_timer_seconds = update_timer_seconds;
      
    }

    /*
     * Our virtual destructor.
     */
    infer_resolution_impl::~infer_resolution_impl()
    {
      volk_free(d_buffer1_);
      volk_free(d_buffer2_);
      delete(d_fft);
      delete(d_ifft);
    }

    void infer_resolution_impl::send_pmt_messages(double refresh_rate, long v_visible, long v_blank, long h_visible, long h_blank)
    {
      double calculated_refresh_rate = refresh_rate;

      message_port_pub(
        pmt::mp("refresh_rate"), 
        pmt::cons(
          pmt::mp("refresh_rate"), 
          pmt::from_double(calculated_refresh_rate)
        )
      );

      long calculated_V_visible = v_visible;

      message_port_pub(
        pmt::mp("Vvisible"), 
        pmt::cons(
          pmt::mp("Vvisible"), 
          pmt::from_long(calculated_V_visible)
        )
      );

      long calculated_V_blank = v_blank;

      message_port_pub(
        pmt::mp("Vblank"), 
        pmt::cons(
          pmt::mp("Vblank"), 
          pmt::from_long(calculated_V_blank)
        )
      );

      long calculated_H_visible = h_visible;

      message_port_pub(
        pmt::mp("Hvisible"), 
        pmt::cons(
          pmt::mp("Hvisible"), 
          pmt::from_long(calculated_H_visible)
        )
      );

      long calculated_H_blank = h_blank;

      message_port_pub(
        pmt::mp("Hblank"), 
        pmt::cons(
          pmt::mp("Hblank"), 
          pmt::from_long(calculated_H_blank)
        )
      );

    }
    int
    infer_resolution_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      //auto start = gr::high_res_timer_now();

      const gr_complex *in = (const gr_complex *) input_items[0];

      //gr_complex *out_x = ( gr_complex *)volk_malloc(noutput_items, volk_get_alignment() / sizeof(gr_complex));

      d_calculated_fft_size = marinov::fft_get_real_size(noutput_items);

      printf("[RESOLUTION_BLOCK] Calculated fft size with : \t%d noutput_items.\r\n", noutput_items);
      printf("[RESOLUTION_BLOCK] Calculated fft size is: \t%d \r\n", d_calculated_fft_size);

      d_calculated_fft_size = 128;
      gr::fft::fft_complex* fft= new gr::fft::fft_complex(   d_calculated_fft_size, true, 1   );

      if ( !d_end )
      {
          printf("[RESOLUTION_BLOCK] Calculated fft size is: \t%d \r\n", d_calculated_fft_size);

          printf("[RESOLUTION_BLOCK] FFT made an in_buffer of \t%d size.  \r\n", fft->inbuf_length());
          printf("[RESOLUTION_BLOCK] FFT made an out_buffer of \t%d size. \r\n", fft->outbuf_length());
          
          memcpy( fft->get_inbuf(), in,                         d_calculated_fft_size*sizeof(gr_complex) );
          memcpy( d_buffer1_,       fft->get_outbuf(),          d_calculated_fft_size*sizeof(gr_complex) );
          
          fft->execute();
          
          uint16_t i=0;
          while ( i<fft->outbuf_length() )
          {
            //d_buffer1_[0] = 
            i += ceil(fft->inbuf_length() / 4);
          }

          printf("[RESOLUTION_BLOCK] Input to fft: %f \r\n", (float)abs(in[64]) );
          printf("[RESOLUTION_BLOCK] Calculated fft: %f \r\n", (float)abs(d_buffer1_[64]) );
          d_end=1;
      }

      long period_ms = d_update_timer_seconds*1000;
      boost::this_thread::sleep(  boost::posix_time::milliseconds(static_cast<long>(period_ms)) );
      d_end = 0;
      //auto end = gr::now();
      
      //float duration = end-start;
      
      //printf("Work Duration : %f \r\n", duration);
      //d_end ++;
      delete(fft);
      return 0;
    }

  } /* namespace tempest */
} /* namespace gr */

