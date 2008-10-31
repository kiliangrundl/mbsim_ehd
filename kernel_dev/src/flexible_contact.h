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
#ifndef _FLEXIBLE_CONTACT_H_
#define _FLEXIBLE_CONTACT_H_

#include "contact.h"
#include "constitutive_laws.h"

namespace MBSim {

  /*! \brief Class for flexible contacts
   *
   * */
  class FlexibleContact : public Contact {

    protected:

      RegularizedConstraintLaw *fcl;
      RegularizedFrictionLaw *ffl;

    public: 
      FlexibleContact(const string &name);

      void init();

      void updateh(double t);

      void setContactLaw(RegularizedConstraintLaw *fcl_) {fcl = fcl_;}
      void setFrictionLaw(RegularizedFrictionLaw *ffl_) {ffl = ffl_;}

      int getFrictionDirections() {return ffl ? ffl->getFrictionDirections() : 0;}

      string getType() const {return "FlexibleContact";}

      bool isClosed() const { return fcl->isClosed(g(0));}
      bool remainsClosed() const { return fcl->remainsClosed(gd(0));}
      bool isSticking() const { return ffl->isSticking(gd(iT));}
      void updateCondition() {}
   // bool isActive() const { return fcl->isClosed(g(0));}
      //void setMarginalVelocity(double v) {gdT_grenz = v;}
  };

}

#endif
