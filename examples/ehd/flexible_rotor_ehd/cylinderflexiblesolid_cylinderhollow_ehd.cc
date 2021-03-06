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
 *          rzander@users.berlios.de
 */

#include "cylinderflexiblesolid_cylinderhollow_ehd.h"

#include "mbsimFlexibleBody/contours/cylinder_flexible.h"
#include "mbsim/contours/frustum.h"
#include "mbsim/functions_contact.h"
#include "mbsim/utils/eps.h"
#include "mbsim/utils/rotarymatrices.h"
#include <mbsim/mbsim_event.h>

using namespace fmatvec;
using namespace MBSim;
using namespace MBSimFlexibleBody;
using namespace std;

namespace MBSimEHD {

  class PointHelper : public MBSim::Point {
    public:
      PointHelper(const fmatvec::Vec3 & WrP) :
          Point() {
        this->getFrameOfReference()->getPosition() = WrP;
      }
  };

  ContactKinematicsCylinderFlexibleSolidCylinderHollowEHD::~ContactKinematicsCylinderFlexibleSolidCylinderHollowEHD() {
  }

  void ContactKinematicsCylinderFlexibleSolidCylinderHollowEHD::assignContours(const vector<Contour*> &contour) {
    if (dynamic_cast<Frustum*>(contour[0])) {
      ihollow = 0;
      isolid = 1;
    }
    else {
      isolid = 0;
      ihollow = 1;
    }

    solid = static_cast<CylinderFlexible*>(contour[isolid]);
    hollow = static_cast<Frustum*>(contour[ihollow]);

    // Check the sizes of the radii (must be the same to be a cylinder and save them
    rSolid = solid->getRadius();

    Vec2 rh = hollow->getRadii();
    if (rh(1) - rh(0) > macheps()) {
      throw MBSimError("The two radii of the hollow must match to be cylinder!");
    }
    else {
      rHollow = rh(0);
    }

    if (rSolid >= rHollow) {
      throw MBSimError("Radius of the solid must be smaller than radius of the hollow!");
    }

    //initialize values
    Vec2 h;
    h(1) = rHollow; //is h2 in [1]
    for (int i = 0; i < numberOfPotentialContactPoints; i++) {
      heightsPos.push_back(h);
      dheightsdyPos.push_back(Vec2());
    }

  }

  void ContactKinematicsCylinderFlexibleSolidCylinderHollowEHD::updateKinematics(const vector<SingleContact> & contacts) {
    Vec3 r; //position of node in F-coordinate system

    int posCounter = 0;
    for (int i = 0; i < numberOfPotentialContactPoints; i++) {

      ContourPointData * cpData = contacts[i].getcpData();
      cpData[ihollow].getLagrangeParameterPosition()(0) = pos(posCounter++); // z or height direction
      cpData[ihollow].getLagrangeParameterPosition()(1) = pos(posCounter++); // phi or azimuthal direction

      const double & z = cpData[ihollow].getLagrangeParameterPosition()(1);

      updatePointKinematics(cpData[isolid], cpData[ihollow]);
    }
  }

  fmatvec::VecV ContactKinematicsCylinderFlexibleSolidCylinderHollowEHD::updateKinematics(const fmatvec::Vec2 & alpha, MBSim::Frame::Feature ff) {
    //The frame feature is ignored by now and all values are computed every time!
    VecV ret(10);

    if (dimLess) {
      throw MBSimError("The dimless case is not implemented for ContactKinematicsCylinderFlexibleSolidCylinderHollowEHD");
    }

    // reference to the entries in the vector
    double & h1 = ret(0);
    double & h2 = ret(1);
    double & h1dy = ret(2);
    double & h2dy = ret(3);
    double & u1 = ret(4);
    double & u2 = ret(5);
    double & v1 = ret(6);
    double & v2 = ret(7);
    double & v1dy = ret(8);
    double & v2dy = ret(9);

    ContourPointData cpHollow(alpha);
    ContourPointData cpSolid(alpha);

    updatePointKinematics(cpSolid, cpHollow);

    Vec3 WrD = cpHollow.getFrameOfReference().getPosition() - cpSolid.getFrameOfReference().getPosition();

    // Compute distances h1 and h2
    //TODO: ...
    h1 = cpSolid.getFrameOfReference().getOrientation().col(0).T() * cpSolid.getFrameOfReference().getPosition();
    h2 = rHollow;

    // Compute derivatives of h1 and h2 with respect to y
//    h1dy = et * h1 / (rHollow * r1); //TODO: should be similar to the cylinder-cylinder case!

    // Check if the film thickness is smaller than zero, meaning
    // if there is a penetration of the contacting bodies
    if (h2 < h1)
      throw MBSimError("Film thickness is smaller than zero, i.e. h < 0.");

    if (dimLess) {
      h1 /= hrF;
      h2 /= hrF;
      h1dy = h1dy * xrF / hrF;
//      h2dy = h2dy * xrF / hrF;
    }

    Vec3 r;
    r(0) = h1;
    r(1) = alpha(1);

    //TODO: update the velocities and then compute the surface velocities!
    solid->updateKinematicsForFrame(cpSolid, Frame::velocities);

    r = hollow->getFrameOfReference()->getPosition() + AIF * r - solid->getFrameOfReference()->getPosition();

    //Fu1 and Fu2 are the velocities on the surfaces in the F-directions 1 and 2
    Vec3 Fu1 = solid->getFrameOfReference()->getVelocity();
    Fu1 += crossProduct(solid->getFrameOfReference()->getAngularVelocity(), r);
    Fu1 = AIF.T() * Fu1; //Transform in F-system

    Vec3 Fu2 = AIF.T() * (hollow->getFrameOfReference()->getVelocity() + crossProduct(hollow->getFrameOfReference()->getAngularVelocity(), AIF.col(0) * h2));

    u1 = Fu1(0);
    v1 = Fu1(2);

// Compute velocities on inner bearing shell surface
    u2 = Fu2(0);
    v2 = Fu2(2);

    if (dimLess) {
      v1dy = v1dy * xrF;
      v2dy = v2dy * xrF;
    }

    return ret;
  }

  void ContactKinematicsCylinderFlexibleSolidCylinderHollowEHD::updatePointKinematics(MBSim::ContourPointData & cpSolid, MBSim::ContourPointData & cpHollow) {
    // compute the rotation matrices
    phi = cpHollow.getLagrangeParameterPosition()(1);
    AKF = BasicRotAKIy(phi);
    AIF = hollow->getFrameOfReference()->getOrientation() * AKF;

    hollow->updateKinematicsForFrame(cpHollow, Frame::position);

    /* longitudinal contact search */
    bool searchLongitudinal = false;
    // in case the hollow and the solid to share the same coordinate length coordinate direction
    // (and no longitudinal relative movement is done)
    // then no longitudinal search is needed!!

    if (searchLongitudinal) {
      PointHelper ptContour(cpHollow.getFrameOfReference().getPosition());
      FuncPairContour1sPoint func(&ptContour, solid);
      Contact1sSearch search(&func);
      search.setNodes(nodes);

      if (true) {
        //local search
        search.setInitialValue(cpHollow.getLagrangeParameterPosition()(0));
      }
      else {
        search.setSearchAll(true);

      }
      cpSolid.getLagrangeParameterPosition()(0) = search.slv();
    }
    else {
      cpSolid.getLagrangeParameterPosition()(0) = cpHollow.getLagrangeParameterPosition()(0);
    }

    // kinematics for azimuthal contact search
    solid->updateKinematicsForFrame(cpSolid, Frame::position_cosy);

    // distance between the surface of the hollow and the center-line point of the solid
    Vec3 WrD = cpHollow.getFrameOfReference().getPosition() - cpSolid.getFrameOfReference().getPosition();

    //TODO this is not the complete correct computation
    Vec3 n = WrD / nrm2(WrD);

    // update the position at the surface of the solid
    cpSolid.getFrameOfReference().getPosition() += n * rSolid;

    //Orientation
    cpHollow.getFrameOfReference().getOrientation().set(0, -AIF.col(0));
    cpHollow.getFrameOfReference().getOrientation().set(1, hollow->getFrameOfReference()->getOrientation().col(1)); //TODO holds not for frustum, only cylinder
    cpHollow.getFrameOfReference().getOrientation().set(2, crossProduct(cpHollow.getFrameOfReference().getOrientation().col(0), cpHollow.getFrameOfReference().getOrientation().col(1)));

    cpSolid.getFrameOfReference().getOrientation().set(0, -cpHollow.getFrameOfReference().getOrientation().col(0));
    cpSolid.getFrameOfReference().getOrientation().set(1, -cpHollow.getFrameOfReference().getOrientation().col(1));
    cpSolid.getFrameOfReference().getOrientation().set(2, cpHollow.getFrameOfReference().getOrientation().col(2));
  }

  void ContactKinematicsCylinderFlexibleSolidCylinderHollowEHD::updatewb(Vec &wb, const Vec &g, ContourPointData *cpData) {
//    const Vec3 KrPC1 = circlehol->getFrame()->getOrientation().T() * (cpData[icirclehol].getFrameOfReference().getPosition() - circlehol->getFrame()->getPosition());
//    const double zeta1 = (KrPC1(1) > 0) ? acos(KrPC1(0) / nrm2(KrPC1)) : 2. * M_PI - acos(KrPC1(0) / nrm2(KrPC1));
//    const double sa1 = sin(zeta1);
//    const double ca1 = cos(zeta1);
//    const double r1 = circlehol->getRadius();
//    Vec3 Ks1(NONINIT);
//    Ks1(0) = -r1 * sa1;
//    Ks1(1) = r1 * ca1;
//    Ks1(2) = 0;
//    Vec3 Kt1(NONINIT);
//    Kt1(0) = 0;
//    Kt1(1) = 0;
//    Kt1(2) = 1;
//    const Vec3 s1 = circlehol->getFrame()->getOrientation() * Ks1;
//    const Vec3 t1 = circlehol->getFrame()->getOrientation() * Kt1;
//    Vec3 n1 = crossProduct(s1, t1);
//    n1 /= nrm2(n1);
//    const Vec3 u1 = s1 / nrm2(s1);
//    const Vec3 &R1 = s1;
//    Vec3 KN1(NONINIT);
//    KN1(0) = -sa1;
//    KN1(1) = ca1;
//    KN1(2) = 0;
//    const Vec3 N1 = circlehol->getFrame()->getOrientation() * KN1;
//    Vec3 KU1(NONINIT);
//    KU1(0) = -ca1;
//    KU1(1) = -sa1;
//    KU1(2) = 0;
//    const Vec3 U1 = circlehol->getFrame()->getOrientation() * KU1;
//
//    const Vec3 KrPC2 = circlesol->getFrame()->getOrientation().T() * (cpData[icirclesol].getFrameOfReference().getPosition() - circlesol->getFrame()->getPosition());
//    const double zeta2 = (KrPC2(1) > 0) ? acos(KrPC2(0) / nrm2(KrPC2)) : 2. * M_PI - acos(KrPC2(0) / nrm2(KrPC2));
//    const double sa2 = sin(zeta2);
//    const double ca2 = cos(zeta2);
//    const double r2 = circlesol->getRadius();
//    Vec3 Ks2(NONINIT);
//    Ks2(0) = -r2 * sa2;
//    Ks2(1) = r2 * ca2;
//    Ks2(2) = 0;
//    Vec3 Kt2(NONINIT);
//    Kt2(0) = 0;
//    Kt2(1) = 0;
//    Kt2(2) = 1;
//    const Vec3 s2 = circlesol->getFrame()->getOrientation() * Ks2;
//    const Vec3 t2 = circlesol->getFrame()->getOrientation() * Kt2;
//    Vec3 n2 = -crossProduct(s2, t2);
//    n2 /= nrm2(n2);
//    const Vec3 u2 = s2 / nrm2(s2);
//    const Vec3 v2 = crossProduct(n2, u2);
//    const Vec3 &R2 = s2;
//    Vec3 KU2(NONINIT);
//    KU2(0) = -ca2;
//    KU2(1) = -sa2;
//    KU2(2) = 0;
//    const Vec3 U2 = circlesol->getFrame()->getOrientation() * KU2;
//
//    const Vec3 vC1 = cpData[icirclehol].getFrameOfReference().getVelocity();
//    const Vec3 vC2 = cpData[icirclesol].getFrameOfReference().getVelocity();
//    const Vec3 Om1 = cpData[icirclehol].getFrameOfReference().getAngularVelocity();
//    const Vec3 Om2 = cpData[icirclesol].getFrameOfReference().getAngularVelocity();
//
//    SqrMat A(2, NONINIT); // TODO: change to FSqrMat
//    A(0, 0) = -(u1.T() * R1);
//    A(0, 1) = u1.T() * R2;
//    A(1, 0) = u2.T() * N1;
//    A(1, 1) = n1.T() * U2;
//    Vec b(2, NONINIT);
//    b(0) = -(u1.T() * (vC2 - vC1));
//    b(1) = -(v2.T() * (Om2 - Om1));
//    const Vec zetad = slvLU(A, b);
//
//    const Mat3x3 tOm1 = tilde(Om1);
//    const Mat3x3 tOm2 = tilde(Om2);
//
//    wb(0) += ((vC2 - vC1).T() * N1 - n1.T() * tOm1 * R1) * zetad(0) + n1.T() * tOm2 * R2 * zetad(1) - n1.T() * tOm1 * (vC2 - vC1);
//    if (wb.size() > 1)
//      wb(1) += ((vC2 - vC1).T() * U1 - u1.T() * tOm1 * R1) * zetad(0) + u1.T() * tOm2 * R2 * zetad(1) - u1.T() * tOm1 * (vC2 - vC1);
//  }
  }

  CylinderFlexible * ContactKinematicsCylinderFlexibleSolidCylinderHollowEHD::getSolidContour() {
    return solid;
  }
  Frustum * ContactKinematicsCylinderFlexibleSolidCylinderHollowEHD::getHollowContour() {
    return hollow;
  }
}

