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

#include "circlesolid_circlehollow_ehd.h"
#include "mbsim/contour.h"
#include "mbsim/contours/circle_solid.h"
#include "mbsim/contours/circle_hollow.h"

using namespace fmatvec;
using namespace std;

namespace MBSim {

  void ContactKinematicsCircleSolidCircleHollowEHD::assignContours(const vector<Contour*> &contour) {
    if (dynamic_cast<CircleSolid*>(contour[0])) {
      icirclesol = 0;
      icirclehol = 1;
    }
    else {
      icirclesol = 1;
      icirclehol = 0;
    }
    circlesol = static_cast<CircleSolid*>(contour[icirclesol]);
    circlehol = static_cast<CircleHollow*>(contour[icirclehol]);
  }

  void ContactKinematicsCircleSolidCircleHollowEHD::updateg(Vec &g, ContourPointData *cpData, int index) {
    WrD = circlesol->getFrame()->getPosition() - circlehol->getFrame()->getPosition();
    WrDdot = circlesol->getFrame()->getVelocity() - circlehol->getFrame()->getVelocity();
    omegaRel = circlesol->getFrame()->getAngularVelocity() - circlehol->getFrame()->getAngularVelocity();

    // TODO: the following is not correct for the general case
    cpData[icirclehol].getFrameOfReference().getPosition() = circlehol->getFrame()->getPosition();
    cpData[icirclesol].getFrameOfReference().getPosition() = circlesol->getFrame()->getPosition();
    g(0) = circlesol->getRadius() - circlehol->getRadius();
    if (nrm2(WrD) < macheps()) {
      cpData[icirclesol].getFrameOfReference().setOrientation(circlesol->getFrame()->getOrientation());
    }
    else {
      cpData[icirclesol].getFrameOfReference().getOrientation().set(0, WrD / nrm2(WrD));
      cpData[icirclesol].getFrameOfReference().getOrientation().set(2, circlehol->getFrame()->getOrientation().col(2));
      cpData[icirclesol].getFrameOfReference().getOrientation().set(1, crossProduct(cpData[icirclesol].getFrameOfReference().getOrientation().col(2), cpData[icirclesol].getFrameOfReference().getOrientation().col(0)));
    }
    cpData[icirclehol].getFrameOfReference().getOrientation().set(0, -cpData[icirclesol].getFrameOfReference().getOrientation().col(0));
    cpData[icirclehol].getFrameOfReference().getOrientation().set(1, -cpData[icirclesol].getFrameOfReference().getOrientation().col(1));
    cpData[icirclehol].getFrameOfReference().getOrientation().set(2, circlesol->getFrame()->getOrientation().col(2));
  }

  void ContactKinematicsCircleSolidCircleHollowEHD::updatewb(Vec &wb, const Vec &g, ContourPointData *cpData) {
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

  fmatvec::Vec3 ContactKinematicsCircleSolidCircleHollowEHD::getWrD() {
    return WrD;
  }
  fmatvec::Vec3 ContactKinematicsCircleSolidCircleHollowEHD::getWrDdot() {
    return WrDdot;
  }
  fmatvec::Vec3 ContactKinematicsCircleSolidCircleHollowEHD::getomegaRel() {
    return omegaRel;
  }
  CircleSolid * ContactKinematicsCircleSolidCircleHollowEHD::getcirclesol() {
    return circlesol;
  }
  CircleHollow * ContactKinematicsCircleSolidCircleHollowEHD::getcirclehol() {
    return circlehol;
  }
}

