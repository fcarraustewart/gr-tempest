/* -*- c++ -*- */
/*!
 * Copyright 2021
 *    Pablo Bertrand    <pablo.bertrand@fing.edu.uy>
 *    Felipe Carrau     <felipe.carrau@fing.edu.uy>
 *    Victoria Severi   <maria.severi@fing.edu.uy>
 *    
 *    Instituto de Ingeniería Eléctrica, Facultad de Ingeniería,
 *    Universidad de la República, Uruguay.
 * 
 * This software is based on Martin Marinov's TempestSDR.
 * In particular, gaussianblur() function is entirely his, and
 * find_best_beta and find_shift are implementations of
 * findbestfit and findthesweetspot, respectively.
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
 * \file sync_detector_impl.h
 * 
 * \brief Block ment to sustitute Hsync in the detection of
 * syncronization of an image with already corrected sampling.
 * The average value of both the horizontal and vertical lines
 * of a frame are used in order to detect the location of the 
 * blankings, and display the samples with the appropriate
 * delay to synchronize them with the margins.
 *
 * \ingroup tempest
 *
 * \date May 3, 2021
 * \author  Pablo Bertrand   <pablo.bertrand@fing.edu.uy>
 * \author  Felipe Carrau    <felipe.carrau@fing.edu.uy>
 * \author  Victoria Severi  <maria.severi@fing.edu.uy>
 */

/**********************************************************
 * Constant and macro definitions
 **********************************************************/

#ifndef INCLUDED_SYNC_DETECTOR_IMPL_H
#define INCLUDED_SYNC_DETECTOR_IMPL_H

/**********************************************************
 * Include statements
 **********************************************************/

#include <tempest/sync_detector.h>

namespace gr {
  namespace tempest {

    class sync_detector_impl : public sync_detector
    {
     private:

      /**********************************************************
       * Data declarations
       **********************************************************/

      //Input data
      int d_hdisplay;
      int d_vdisplay;
      int d_hblanking;
      int d_vblanking;
      int d_Htotal;
      int d_Vtotal;

      //Blanking variables
      int d_blanking_size_h;
      int d_blanking_size_v;
      int d_blanking_index_h;
      int d_blanking_index_v;
      int d_working_index_h;

      //Flags
      int d_frame_average_complete;
      int d_frame_wait_for_blanking;
      int d_frame_output;


      //Counters
      int d_frame_height_counter; 
      int d_blanking_wait_counter; 
      int d_output_counter;

      // Control flag, and its mutex
      gr::thread::mutex d_mutex;
      bool d_start_sync_detect;   

      //Arrays
      float * d_data_h;
      float * d_avg_h_line;
      float * d_avg_v_line;

      //Fixed parameters
      float d_LOWPASS_COEFF_V;
      float d_LOWPASS_COEFF_H;
      float d_GAUSSIAN_ALPHA;

      /**********************************************************
       * Private function prototypes
       **********************************************************/
      
      /*!
        * \brief Function that takes a full line (either horizontal 
        * or vertical) and a fixed blanking size to calculate the 
        * medium energy difference between blanking and screen (beta)
        * for each possible blanking position. Returns the position 
        * that granted the best beta and the value of beta itself.
        * 
        * \param const float *data: array contaning the average
        * magnitudes of either the horizontal or vertical lines.
        * \param const int total_line_size: data array size.
        * \param const double total_sum: sum of all array data.
        * \param const int blanking_size: current estimation of
        * the blanking size (horizontal or vertical, according
        * to the data input).
        * \param double *beta: location to store the resulting 
        * beta value.
        * \param int *beta_index: location to store the resulting
        * beta index.
        */
      void find_best_beta (const float *data, const int total_line_size, const double total_sum, const int blanking_size, double *beta, int *beta_index);
      //---------------------------------------------------------
      /*!
        * \brief Function that takes the line and runs find_best_
        * beta for some possible blanking sizes. Then takes the 
        * best location (and size) returned and defines the new 
        * position for shifting using both the new calculation and
        * the previous information to prevent sudden big changes.
        * 
        * \param int *blanking_index: location of the resulting
        * index for the center of the corresponding blanking.
        * \param int *blanking_size: size found for the blanking
        * and used in its calculation.
        * \param float *data: array contaning the average
        * magnitudes of either the horizontal or vertical lines.
        * \param const int total_line_size: data array size.
        * \param int min_blanking_size: minimum blanking allowed
        * to try by the function.
        * \param double lowpasscoeff: coefficient used to affect
        * the set of previous results by each new one.
        */
      void find_shift (int *blanking_index, int *blanking_size, float *data, const int total_line_size, int min_blanking_size, double lowpasscoeff);
      //---------------------------------------------------------
      /*!
        * \brief Marinov's gaussian blur application for a 
        * fixed-size array.
        *  
        * \param float * data: array of data to blur.
        * \param int size: size of data array.
        */
      void gaussianblur(float * data, int size);
      //---------------------------------------------------------
      /*!
        * \@brief Calculation of the gaussian coefficient.
        *  
        */
      float calculate_gauss_coeff(float N, float i);
      //---------------------------------------------------------
      //---------------------------------------------------------
      /*!
        * \brief Message handler for starting or stopping 
        * this block.
        *  
        * \param pmt::pmt_t msg data received.
        */
      void set_ena_msg(pmt::pmt_t msg);

      /**********************************************************
       * Public function prototypes
       **********************************************************/

      public:
      sync_detector_impl(int hscreen, int vscreen, int hblanking, int vblanking);
      ~sync_detector_impl();
      /*!
        * \brief Used to establish the amount of samples required
        * for a full work iteration.
        */
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);
      //---------------------------------------------------------
      /*!
        * \brief Function working similarly to a state machine,
        * where the data input (received in horizontal lines) is
        * being used for both calculating the averages needed by
        * the functions, and maintaining the proper delays given
        * by those functions.
        *  
        */
      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
      //---------------------------------------------------------
    };

  } // namespace sync_detector
} // namespace gr

#endif /* INCLUDED_SYNC_DETECTOR_IMPL_H */
