/* Copyright (C) 2004-2015 MBSim Development Team
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
 * Contact: martin.o.foerg@googlemail.com
 */

#ifndef _CONTACT_KINEMATICS_CIRCLESOLID_CIRCLEHOLLOW_EHD_H_
#define _CONTACT_KINEMATICS_CIRCLESOLID_CIRCLEHOLLOW_EHD_H_

#include "mbsim/contact_kinematics/contact_kinematics.h"
#include "mbsim/mbsim_event.h"

namespace MBSim {
  class Frustum;
}

namespace MBSimEHD {

  /*!
   * \brief interface class for all contact kinematics for EHD computation
   */
  class ContactKinematicsEHDInterface : public MBSim::ContactKinematics {

    public:
      // Film thickness and derivatives
      //
      // Input:
      //   sys:    Object of system
      //   y:      Point inside fluid domain (y-coordinate)
      //   e(~):   Element number in spatial discretization
      //   g(~):   Gauss point number in element corresponding to x
      //
      // Output:
      //   h1:     Distance from bearing shell center point to point
      //           on journal surface
      //   h2:     Distance from bearing shell center point to point
      //           on inner bearing shell surface
      //   h1dy:   Derivative of h1 with respect to y
      //   h2dy:   Derivative of h2 with respect to y
      virtual void Thickness(const fmatvec::VecV & x, const int & e, const int & g, double & h1, double & h2, double & h1dy, double & h2dy) = 0;

      // Velocities on journal and inner bearing shell surface and
      // derivatives
      //
      // Input:
      //   sys:    Object of system
      //   x:      Point inside fluid domain y or [y; z]
      //   e(~):   Element number in spatial discretization
      //   g(~):   Gauss point number in element corresponding to x
      //
      // Output:
      //   u1, v1: Velocities on journal surface
      //   u2, v2: Velocities on inner bearing shell surface
      //   v1dy:   Derivative of v1 with respect to y
      //   v2dy:   Derivative of v2 with respect to y
      virtual void Velocities(const fmatvec::VecV & x, const int & e, const int & g, double & u1, double & u2, double & v1, double & v2, double & v1dy, double & v2dy) = 0;


      /*!
       * \brief retrieve characteristic size for film thickness
       */
      double gethrF() {
        return hrF;
      }

      /*!
       * \brief retrieve characteristic size for fluid domain
       */
      double getxrF() {
        return xrF;
      }


      void setdimless(bool dimless_) {
        dimLess = dimless_;
      }

      bool getdimless() {
        return dimLess;
      }

    protected:
      /*!
       * \brief Flag for dimensionless description
       */
      bool dimLess;

      /*!
       * \brief Characteristic size for fluid domain
       */
      double xrF;

      /*!
       * \brief Characteristic size for film thickness
       */
      double hrF;

  };

  /**
   * \brief pairing circle outer side to circle inner side for EHD contacts
   * \author Kilian Grundl
   */
  class ContactKinematicsCylinderSolidCylinderHollowEHD : public ContactKinematicsEHDInterface {
    public:
      /* INHERITED INTERFACE */
      virtual void assignContours(const std::vector<MBSim::Contour*> &contour);
      virtual void updateg(fmatvec::Vec &g, MBSim::ContourPointData *cpData, int index = 0);
      virtual void updatewb(fmatvec::Vec &wb, const fmatvec::Vec &g, MBSim::ContourPointData *cpData);
      virtual void Thickness(const fmatvec::VecV & x, const int & e, const int & g, double & h1, double & h2, double & h1dy, double & h2dy);
      virtual void Velocities(const fmatvec::VecV & x, const int & e, const int & g, double & u1, double & u2, double & v1, double & v2, double & v1dy, double & v2dy);
      /***************************************************/

      fmatvec::Vec3 getWrD();
      fmatvec::Vec3 getWrDdot();
      fmatvec::Vec3 getomegaRel();

      MBSim::Frustum * getcirclesol();
      MBSim::Frustum * getcirclehol();

    private:
      /**
       * \brief contour index
       */
      int isolid, ihollow;

      /**
       * \brief contour classes
       */
      MBSim::Frustum *solid;
      MBSim::Frustum *hollow;

      /*!
       * \brief the radius of the solid
       */
      double rSolid;

      /*
       * \brief the radius of the hollow
       */
      double rHollow;

      /*!
       * \brief distance between the center points
       */
      fmatvec::Vec3 WrD;

      /*!
       * \brief relative velocity between the center points
       */
      fmatvec::Vec3 WrDdot;

      /*!
       * \brief relative angular velocity
       */
      fmatvec::Vec3 omegaRel;

      /*
       * \brief Radial eccentricity in coordinate system F
       */
      double er;

      /*!
       * \brief Tangential eccentricity in coordinate system F
       */
      double et;

      /*
       * \brief Auxiliary length variable
       */
      double r1;

    private:
      // Radial and tangential eccentricity in coordinate system F
      // and auxiliary length variable
      //
      // Input:
      //   y:      Coordinate of fluid domain
      void Eccentricity(const double & y);

      // Mapping between y-coordinate of fluid domain (unwrapped
      // inner bearing shell surface) and angle phi of rotated
      // coordinate system F
      //
      // Input:
      //   sys:    Object of system
      //   y:      Coordinate of fluid domain
      //
      // Output:
      //   phi:    Angle of rotated coordinate system F
      //   AFK:    Rotation matrix
      //TODO: fit to MBSim SqrMat3 etc. --> use the mbsim functions!!
      void AngleCoordSys(const double & y, double & phi, fmatvec::SqrMat2 & AFK);

  };

}

#endif
