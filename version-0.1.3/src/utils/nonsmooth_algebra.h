/* Copyright (C) 2004-2006  Martin Förg
 
 * This library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 *  
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details. 
 *  
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this library; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

 *
 * Contact:
 *   mfoerg@users.berlios.de
 *
 */
#ifndef NONSMOOTH_ALGEBRA_H_
#define NONSMOOTH_ALGEBRA_H_

#include "fmatvec.h"

using namespace fmatvec;

namespace MBSim {
  /*! prox-function for normal direction */
  double proxCN(double arg);
  /*! prox-function for 2D-tangential direction */
  double proxCT2D(double arg, double LaNmue);
  /*! prox-function for 3D-tangential direction */
  Vec proxCT3D(const Vec& arg, double laNmue);
  /*! prox-function for normal direction with translated cone */
  double proxCN(double arg,double boundary);

}

#endif /* NONSMOOTH_ALGEBRA_H_ */