/* -*- c++ -*- */
/*
 * Copyright 2021
 *    Pablo Bertrand    <pablo.bertrand@fing.edu.uy>
 *    Felipe Carrau     <felipe.carrau@fing.edu.uy>
 *    Victoria Severi   <maria.severi@fing.edu.uy>
 *    
 *    Instituto de Ingeniería Eléctrica, Facultad de Ingeniería,
 *    Universidad de la República, Uruguay.
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

#ifndef INCLUDED_TEMPEST_SYNC_DETECTOR_H
#define INCLUDED_TEMPEST_SYNC_DETECTOR_H

#include <tempest/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace tempest {

    class TEMPEST_API sync_detector : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<sync_detector> sptr;

      static sptr make(int hscreen, int vscreen, int hblanking, int vblanking);
    };

  } // namespace tempest
} // namespace gr

#endif /* INCLUDED_TEMPEST_SYNC_DETECTOR_H */

