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
 * @file normalize_flow_impl.h
 * 
 * @brief Block that takes max and min values and uses them
 * to normalize the modules of incoming samples.
 *
 * gr-tempest
 *
 * @date May 26, 2020
 * @author Federico "Larroca" La Rocca <flarroca@fing.edu.uy>
 */

/**********************************************************
 * Constant and macro definitions
 **********************************************************/

#ifndef INCLUDED_TEMPEST_NORMALIZE_FLOW_IMPL_H
#define INCLUDED_TEMPEST_NORMALIZE_FLOW_IMPL_H

/**********************************************************
 * Include statements
 **********************************************************/

#include <tempest/normalize_flow.h>
#include <random>

namespace gr {
  namespace tempest {

    class normalize_flow_impl : public normalize_flow
    {
     private:

      /**********************************************************
       * Data declarations
       **********************************************************/

      float d_min; 
      float d_max; 
      int   d_win; 
      float d_alpha_avg; 

      float d_current_max;
      float d_current_min;

      float * d_minus_datain; 

      // to accelerate the process, I'll only update the interpolation
      // once every a random number of iterations with probability d_portion
      std::uniform_real_distribution<float> d_dist;
      std::minstd_rand d_gen;
      float d_proba_of_updating;
      
      /**********************************************************
       * Private function prototypes
       **********************************************************/
      
      /**
        * @brief Receives an array of floats, looks for the index
        * of the maximum value and returns the value itself.
        * 
        * @param const float *datain: contains the array of data.
        * @param const int datain_length: establishes array length.
        */
      float compute_max(const float * datain, const int datain_length);
      //---------------------------------------------------------

      /**********************************************************
       * Public function prototypes
       **********************************************************/

      public:
      normalize_flow_impl(float min, float max, int window, float alpha_avg, float update_proba);
      ~normalize_flow_impl();
      /**
        * @brief Randomly uses the noutput_items received to update
        * the maximum and minimum values. In every iteration, all
        * samples are normalized according to an ecuation that uses
        * both mentioned values.
        *  
        */
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
      //---------------------------------------------------------
      /**
        * @brief Initializes variables used for min and max in the
        * normalization process. Operates with callback to allow
        * changes during execution.
        *  
        * @param float min: variable used for minimum.
        * @param float max: variable used for maximum.
        */
      void set_min_max(float min, float max);
      //---------------------------------------------------------
    };

  } // namespace tempest
} // namespace gr

#endif /* INCLUDED_TEMPEST_NORMALIZE_FLOW_IMPL_H */

