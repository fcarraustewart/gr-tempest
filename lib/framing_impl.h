/* -*- c++ -*- */
/**
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
 * @file framing_impl.h
 * 
 * @brief Block that determines output per line according to 
 * the line count made in reference to the vertical display
 * size.
 *
 * gr-tempest
 *
 * @date May 16, 2020
 * @author Federico "Larroca" La Rocca <flarroca@fing.edu.uy>
 */

/**********************************************************
 * Constant and macro definitions
 **********************************************************/

#ifndef INCLUDED_TEMPEST_FRAMING_IMPL_H
#define INCLUDED_TEMPEST_FRAMING_IMPL_H

/**********************************************************
 * Include statements
 **********************************************************/

#include <tempest/framing.h>

namespace gr {
  namespace tempest {

    class framing_impl : public framing
    {
      private:

      /**********************************************************
       * Data declarations
       **********************************************************/

      int d_Htotal; 
      int d_Vtotal; 
      int d_Hdisplay; 
      int d_Vdisplay; 
      int d_current_line; 
      float * d_zeros;

      /**********************************************************
       * Public function prototypes
       **********************************************************/
      
      public:
      framing_impl(int Htotal, int Vtotal, int Hdisplay, int Vdisplay);
      ~framing_impl();
      /**
        * @brief Initializes variables used for horizontal and 
        * vertical length. Operates with callback to allow changes 
        * during execution.
        *  
        */
      void set_Htotal_and_Vtotal(int Htotal, int Vtotal);
      //---------------------------------------------------------
      /**
        * @brief Used to establish the amount of samples required
        * for a full work iteration.
        */
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);
      //---------------------------------------------------------
      /**
        * @brief Receives samples in horizontal lines and counts
        * them. Up until the vertical display size, lines are
        * either copied in the output if they are within the
        * total vertical size, or else replaced by zeros.
        *  
        */
      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
      //---------------------------------------------------------
    };

  } // namespace tempest
} // namespace gr

#endif /* INCLUDED_TEMPEST_FRAMING_IMPL_H */

