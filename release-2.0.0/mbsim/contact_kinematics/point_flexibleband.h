/* Copyright (C) 2004-2009 MBSim Development Team
 *
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
 * Contact: thschindler@users.berlios.de
 */

#ifndef POINT_FLEXIBLEBAND_H_
#define POINT_FLEXIBLEBAND_H_

#include "mbsim/contact_kinematics/contact_kinematics.h"
#include "mbsim/mbsim_event.h"

namespace MBSim {

  class FlexibleBand;
  class Point;

  /**
   * \brief pairing point to flexible band
   * \author Thorsten Schindler
   * \author Roland Zander
   * \date 2009-04-17 initial commit kernel_dev (Thorsten Schindler) 
   */
  class ContactKinematicsPointFlexibleBand : public ContactKinematics {
    public:
      /**
       * \brief constructor
       */
      ContactKinematicsPointFlexibleBand();
      
      /**
       * \brief destructor
       */
      virtual ~ContactKinematicsPointFlexibleBand();

      /* INHERITED INTERFACE OF CONTACTKINEAMTICS */
      virtual void assignContours(const std::vector<Contour*>& contour);
      virtual void updateg(fmatvec::Vec& g, ContourPointData *cpData);   
      virtual void updatewb(fmatvec::Vec& wb, const fmatvec::Vec &g,ContourPointData *cpData) { throw new MBSimError("ERROR (ContactKinematicsPointFlexibleBand::updatewb): not implemented!"); }   
      /***************************************************/

    private:
      /** 
       * \brief contour index 
       */
      int ipoint, icontour;

      /** 
       * \brief contour classes 
       */
      Point *point;
      FlexibleBand *band;
  };

}

#endif /* POINT_FLEXIBLEBAND_H_ */
