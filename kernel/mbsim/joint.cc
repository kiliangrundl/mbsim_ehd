/* Copyright (C) 2004-2014 MBSim Development Team
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

#include <config.h>
#include "mbsim/joint.h"
#include "mbsim/constitutive_laws.h"
#include "mbsim/dynamic_system_solver.h"
#include "mbsim/objectfactory.h"
#include "mbsim/contact_kinematics/contact_kinematics.h"
#include <mbsim/utils/utils.h>
#include <mbsim/rigid_body.h>

using namespace std;
using namespace fmatvec;
using namespace MBXMLUtils;
using namespace xercesc;
using namespace boost;

namespace MBSim {

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(Joint, MBSIM%"Joint")

  Joint::Joint(const string &name) :
      LinkMechanics(name), refFrame(NULL), refFrameID(0), ffl(0), fml(0), fifl(0), fiml(0), C("C") {
  }

  Joint::~Joint() {
    if (ffl) {
      delete ffl;
      ffl = 0;
    }
    if (fml) {
      delete fml;
      fml = 0;
    }
    if (fifl) {
      delete fifl;
      fifl = 0;
    }
    if (fiml) {
      delete fiml;
      fiml = 0;
    }
  }

  void Joint::connect(Frame* frame0, Frame* frame1) {
    LinkMechanics::connect(frame0);
    LinkMechanics::connect(frame1);
  }

  void Joint::updatewb(double t, int j) {
    Mat3xV WJT = refFrame->getOrientation() * JT;
    VecV sdT = WJT.T() * (WvP0P1);

    wb(0, Wf.cols() - 1) += Wf.T() * (frame[1]->getGyroscopicAccelerationOfTranslation(j) - C.getGyroscopicAccelerationOfTranslation(j) - crossProduct(C.getAngularVelocity(), WvP0P1 + WJT * sdT));
    wb(Wf.cols(), Wm.cols() + Wf.cols() - 1) += Wm.T() * (frame[1]->getGyroscopicAccelerationOfRotation(j) - C.getGyroscopicAccelerationOfRotation(j) - crossProduct(C.getAngularVelocity(), WomP0P1));
  }

  void Joint::updateW(double t, int j) {
    fF[1].set(Index(0, 2), Index(0, Wf.cols() - 1), Wf);
    fM[1].set(Index(0, 2), Index(Wf.cols(), Wf.cols() + Wm.cols() - 1), Wm);
    fF[0] = -fF[1];
    fM[0] = -fM[1];

    W[j][0] += C.getJacobianOfTranslation(j).T() * fF[0] + C.getJacobianOfRotation(j).T() * fM[0];
    W[j][1] += frame[1]->getJacobianOfTranslation(j).T() * fF[1] + frame[1]->getJacobianOfRotation(j).T() * fM[1];
  }

  void Joint::updateh(double t, int j) {
    if (ffl) {
      if (not ffl->isSetValued()) {
        for (int i = 0; i < forceDir.cols(); i++)
          la(i) = (*ffl)(g(i), gd(i));
      }
    }

    if (fml) {
      if (not fml->isSetValued()) {
        for (int i = forceDir.cols(); i < forceDir.cols() + momentDir.cols(); i++)
          la(i) = (*fml)(g(i), gd(i));
      }
    }

    WF[1] = Wf * la(IT);
    WM[1] = Wm * la(IR);
    WF[0] = -WF[1];
    WM[0] = -WM[1];
    h[j][0] += C.getJacobianOfTranslation(j).T() * WF[0] + C.getJacobianOfRotation(j).T() * WM[0];
    h[j][1] += frame[1]->getJacobianOfTranslation(j).T() * WF[1] + frame[1]->getJacobianOfRotation(j).T() * WM[1];
  }

  void Joint::updateg(double t) {
    Wf = refFrame->getOrientation() * forceDir;
    Wm = refFrame->getOrientation() * momentDir;

    WrP0P1 = frame[1]->getPosition() - frame[0]->getPosition();
    C.setOrientation(refFrame->getOrientation());
    C.setPosition(frame[0]->getPosition() + WrP0P1);

    g(IT) = Wf.T() * WrP0P1;
    g(IR) = x;
  }

  void Joint::updategd(double t) {
    C.setAngularVelocity(frame[0]->getAngularVelocity());
    C.setVelocity(frame[0]->getVelocity() + crossProduct(frame[0]->getAngularVelocity(), WrP0P1));

    WvP0P1 = frame[1]->getVelocity() - C.getVelocity();
    WomP0P1 = frame[1]->getAngularVelocity() - C.getAngularVelocity();

    gd(IT) = Wf.T() * WvP0P1;
    gd(IR) = Wm.T() * WomP0P1;
  }

  void Joint::updateJacobians(double t, int j) {
    Mat3x3 tWrP0P1 = tilde(WrP0P1);

    C.setJacobianOfTranslation(frame[0]->getJacobianOfTranslation(j) - tWrP0P1 * frame[0]->getJacobianOfRotation(j), j);
    C.setJacobianOfRotation(frame[0]->getJacobianOfRotation(j), j);
    C.setGyroscopicAccelerationOfTranslation(frame[0]->getGyroscopicAccelerationOfTranslation(j) - tWrP0P1 * frame[0]->getGyroscopicAccelerationOfRotation(j) + crossProduct(frame[0]->getAngularVelocity(), crossProduct(frame[0]->getAngularVelocity(), WrP0P1)), j);
    C.setGyroscopicAccelerationOfRotation(frame[0]->getGyroscopicAccelerationOfRotation(j), j);
  }

  void Joint::updatexd(double t) {
    xd = gd(IR);
  }

  void Joint::updatedx(double t, double dt) {
    xd = gd(IR) * dt;
  }

  void Joint::calcxSize() {
    LinkMechanics::calcxSize();
    xSize = momentDir.cols();
  }

  void Joint::init(InitStage stage) {
    if (stage == resolveXMLPath) {
      if (saved_ref1 != "" && saved_ref2 != "")
        connect(getByPath<Frame>(saved_ref1), getByPath<Frame>(saved_ref2));
      if(not(frame.size()))
        THROW_MBSIMERROR("no connection given!");
      LinkMechanics::init(stage);
    }
    else if (stage == resize) {
      LinkMechanics::init(stage);

      g.resize(forceDir.cols() + momentDir.cols());
      gd.resize(forceDir.cols() + momentDir.cols());
      la.resize(forceDir.cols() + momentDir.cols());
      gdd.resize(gdSize);
      gdn.resize(gdSize);
    }
    else if (stage == unknownStage) {
      LinkMechanics::init(stage);

      if (ffl) {
        fifl = new BilateralImpact;
        fifl->setParent(this);
      }
      if (fml) {
        fiml = new BilateralImpact;
        fiml->setParent(this);
      }

      IT = Index(0, forceDir.cols() - 1);
      IR = Index(forceDir.cols(), forceDir.cols() + momentDir.cols() - 1);
      if (forceDir.cols())
        Wf = forceDir;
      else {
      }
      if (momentDir.cols())
        Wm = momentDir;
      else {
      }

      C.getJacobianOfTranslation(0).resize(frame[0]->getJacobianOfTranslation(0).cols());
      C.getJacobianOfRotation(0).resize(frame[0]->getJacobianOfRotation(0).cols());
      C.getJacobianOfTranslation(1).resize(frame[0]->getJacobianOfTranslation(1).cols());
      C.getJacobianOfRotation(1).resize(frame[0]->getJacobianOfRotation(1).cols());

      JT.resize(3 - forceDir.cols());
      if (forceDir.cols() == 2)
        JT.set(0, crossProduct(forceDir.col(0), forceDir.col(1)));
      else if (forceDir.cols() == 3)
        ;
      else if (forceDir.cols() == 0)
        JT = SqrMat(3, EYE);
      else { // define a coordinate system in the plane perpendicular to the force direction
        JT.set(0, computeTangential(forceDir.col(0)));
        JT.set(1, crossProduct(forceDir.col(0), JT.col(0)));
      }
      refFrame = refFrameID ? frame[1] : frame[0];
    }
    else if (stage == plotting) {
      updatePlotFeatures();
      if (getPlotFeature(plotRecursive) == enabled) {
        if (getPlotFeature(generalizedLinkForce) == enabled) {
          for (int j = 0; j < la.size(); ++j)
            plotColumns.push_back("la(" + numtostr(j) + ")");
        }
        if (getPlotFeature(linkKinematics) == enabled) {
          for (int j = 0; j < g.size(); ++j)
            plotColumns.push_back("g(" + numtostr(j) + ")");
          for (int j = 0; j < gd.size(); ++j)
            plotColumns.push_back("gd(" + numtostr(j) + ")");
        }
        LinkMechanics::init(stage);
      }
    }
    else
      LinkMechanics::init(stage);
    if(ffl) ffl->init(stage);
    if(fml) fml->init(stage);
    if(fifl) fifl->init(stage);
    if(fiml) fiml->init(stage);
  }

  void Joint::calclaSize(int j) {
    LinkMechanics::calclaSize(j);
    laSize = forceDir.cols() + momentDir.cols();
  }

  void Joint::calcgSize(int j) {
    LinkMechanics::calcgSize(j);
    gSize = forceDir.cols() + momentDir.cols();
  }

  void Joint::calcgdSize(int j) {
    LinkMechanics::calcgdSize(j);
    gdSize = forceDir.cols() + momentDir.cols();
  }

  void Joint::calcrFactorSize(int j) {
    LinkMechanics::calcrFactorSize(j);
    rFactorSize = isSetValued() ? forceDir.cols() + momentDir.cols() : 0;
  }

  void Joint::calccorrSize(int j) {
    LinkMechanics::calccorrSize(j);
    corrSize = forceDir.cols() + momentDir.cols();
  }

  bool Joint::isSetValued() const {
    bool flag = false;
    if (ffl)
      flag |= ffl->isSetValued();
    if (fml)
      flag |= fml->isSetValued();

    return flag;
  }

  bool Joint::isSingleValued() const {
    bool flag = false;
    if (ffl)
      if (not ffl->isSetValued())
        flag = true;
    if (fml)
      if (not fml->isSetValued())
        flag = true;

    return flag;
  }

  void Joint::solveImpactsFixpointSingle(double dt) {

    const double *a = ds->getGs()();
    const int *ia = ds->getGs().Ip();
    const int *ja = ds->getGs().Jp();
    const Vec &laMBS = ds->getla();
    const Vec &b = ds->getb();

    for (int i = 0; i < forceDir.cols(); i++) {
      gdn(i) = b(laInd + i);
      for (int j = ia[laInd + i]; j < ia[laInd + 1 + i]; j++)
        gdn(i) += a[j] * laMBS(ja[j]);

      la(i) = fifl->project(la(i), gdn(i), gd(i), rFactor(i));
    }
    for (int i = forceDir.cols(); i < forceDir.cols() + momentDir.cols(); i++) {
      gdn(i) = b(laInd + i);
      for (int j = ia[laInd + i]; j < ia[laInd + 1 + i]; j++)
        gdn(i) += a[j] * laMBS(ja[j]);

      la(i) = fiml->project(la(i), gdn(i), gd(i), rFactor(i));
    }
  }

  void Joint::solveConstraintsFixpointSingle() {

    const double *a = ds->getGs()();
    const int *ia = ds->getGs().Ip();
    const int *ja = ds->getGs().Jp();
    const Vec &laMBS = ds->getla();
    const Vec &b = ds->getb();

    for (int i = 0; i < forceDir.cols(); i++) {
      gdd(i) = b(laInd + i);
      for (int j = ia[laInd + i]; j < ia[laInd + 1 + i]; j++)
        gdd(i) += a[j] * laMBS(ja[j]);

      la(i) = ffl->project(la(i), gdd(i), rFactor(i));
    }
    for (int i = forceDir.cols(); i < forceDir.cols() + momentDir.cols(); i++) {
      gdd(i) = b(laInd + i);
      for (int j = ia[laInd + i]; j < ia[laInd + 1 + i]; j++)
        gdd(i) += a[j] * laMBS(ja[j]);

      la(i) = fml->project(la(i), gdd(i), rFactor(i));
    }
  }

  void Joint::solveImpactsGaussSeidel(double dt) {

    const double *a = ds->getGs()();
    const int *ia = ds->getGs().Ip();
    const int *ja = ds->getGs().Jp();
    const Vec &laMBS = ds->getla();
    const Vec &b = ds->getb();

    for (int i = 0; i < forceDir.cols(); i++) {
      gdn(i) = b(laInd + i);
      for (int j = ia[laInd + i] + 1; j < ia[laInd + 1 + i]; j++)
        gdn(i) += a[j] * laMBS(ja[j]);

      la(i) = fifl->solve(a[ia[laInd + i]], gdn(i), gd(i));
    }
    for (int i = forceDir.cols(); i < forceDir.cols() + momentDir.cols(); i++) {
      gdn(i) = b(laInd + i);
      for (int j = ia[laInd + i] + 1; j < ia[laInd + 1 + i]; j++)
        gdn(i) += a[j] * laMBS(ja[j]);

      la(i) = fiml->solve(a[ia[laInd + i]], gdn(i), gd(i));
    }
  }

  void Joint::solveConstraintsGaussSeidel() {

    const double *a = ds->getGs()();
    const int *ia = ds->getGs().Ip();
    const int *ja = ds->getGs().Jp();
    const Vec &laMBS = ds->getla();
    const Vec &b = ds->getb();

    for (int i = 0; i < forceDir.cols(); i++) {
      gdd(i) = b(laInd + i);
      for (int j = ia[laInd + i] + 1; j < ia[laInd + 1 + i]; j++)
        gdd(i) += a[j] * laMBS(ja[j]);

      la(i) = ffl->solve(a[ia[laInd + i]], gdd(i));
    }
    for (int i = forceDir.cols(); i < forceDir.cols() + momentDir.cols(); i++) {
      gdd(i) = b(laInd + i);
      for (int j = ia[laInd + i] + 1; j < ia[laInd + 1 + i]; j++)
        gdd(i) += a[j] * laMBS(ja[j]);

      la(i) = fml->solve(a[ia[laInd + i]], gdd(i));
    }
  }

  void Joint::solveImpactsRootFinding(double dt) {

    const double *a = ds->getGs()();
    const int *ia = ds->getGs().Ip();
    const int *ja = ds->getGs().Jp();
    const Vec &laMBS = ds->getla();
    const Vec &b = ds->getb();

    for (int i = 0; i < forceDir.cols(); i++) {
      gdn(i) = b(laInd + i);
      for (int j = ia[laInd + i]; j < ia[laInd + 1 + i]; j++)
        gdn(i) += a[j] * laMBS(ja[j]);

      res(i) = la(i) - fifl->project(la(i), gdn(i), gd(i), rFactor(i));
    }
    for (int i = forceDir.cols(); i < forceDir.cols() + momentDir.cols(); i++) {
      gdn(i) = b(laInd + i);
      for (int j = ia[laInd + i]; j < ia[laInd + 1 + i]; j++)
        gdn(i) += a[j] * laMBS(ja[j]);

      res(i) = la(i) - fiml->project(la(i), gdn(i), gd(i), rFactor(i));
    }
  }

  void Joint::solveConstraintsRootFinding() {

    const double *a = ds->getGs()();
    const int *ia = ds->getGs().Ip();
    const int *ja = ds->getGs().Jp();
    const Vec &laMBS = ds->getla();
    const Vec &b = ds->getb();

    for (int i = 0; i < forceDir.cols(); i++) {
      gdd(i) = b(laInd + i);
      for (int j = ia[laInd + i]; j < ia[laInd + 1 + i]; j++)
        gdd(i) += a[j] * laMBS(ja[j]);

      res(i) = la(i) - ffl->project(la(i), gdd(i), rFactor(i));
    }
    for (int i = forceDir.cols(); i < forceDir.cols() + momentDir.cols(); i++) {
      gdd(i) = b(laInd + i);
      for (int j = ia[laInd + i]; j < ia[laInd + 1 + i]; j++)
        gdd(i) += a[j] * laMBS(ja[j]);

      res(i) = la(i) - fml->project(la(i), gdd(i), rFactor(i));
    }
  }

  void Joint::jacobianConstraints() {

    const SqrMat Jprox = ds->getJprox();
    const SqrMat G = ds->getG();

    for (int i = 0; i < forceDir.cols(); i++) {
      RowVec jp1 = Jprox.row(laInd + i);
      RowVec e1(jp1.size());
      e1(laInd + i) = 1;
      Vec diff = ffl->diff(la(i), gdd(i), rFactor(i));

      jp1 = e1 - diff(0) * e1; // -diff(1)*G.row(laInd+i)
      for (int j = 0; j < G.size(); j++)
        jp1(j) -= diff(1) * G(laInd + i, j);
    }

    for (int i = forceDir.cols(); i < forceDir.cols() + momentDir.cols(); i++) {

      RowVec jp1 = Jprox.row(laInd + i);
      RowVec e1(jp1.size());
      e1(laInd + i) = 1;
      Vec diff = fml->diff(la(i), gdd(i), rFactor(i));

      jp1 = e1 - diff(0) * e1; // -diff(1)*G.row(laInd+i)
      for (int j = 0; j < G.size(); j++)
        jp1(j) -= diff(1) * G(laInd + i, j);
    }
  }

  void Joint::jacobianImpacts() {

    const SqrMat Jprox = ds->getJprox();
    const SqrMat G = ds->getG();

    for (int i = 0; i < forceDir.cols(); i++) {
      RowVec jp1 = Jprox.row(laInd + i);
      RowVec e1(jp1.size());
      e1(laInd + i) = 1;
      Vec diff = fifl->diff(la(i), gdn(i), gd(i), rFactor(i));

      jp1 = e1 - diff(0) * e1; // -diff(1)*G.row(laInd+i)
      for (int j = 0; j < G.size(); j++)
        jp1(j) -= diff(1) * G(laInd + i, j);
    }

    for (int i = forceDir.cols(); i < forceDir.cols() + momentDir.cols(); i++) {
      RowVec jp1 = Jprox.row(laInd + i);
      RowVec e1(jp1.size());
      e1(laInd + i) = 1;
      Vec diff = fiml->diff(la(i), gdn(i), gd(i), rFactor(i));

      jp1 = e1 - diff(0) * e1; // -diff(1)*G.row(laInd+i)
      for (int j = 0; j < G.size(); j++)
        jp1(j) -= diff(1) * G(laInd + i, j);
    }
  }

  void Joint::updaterFactors() {
    if (isActive()) {

      const double *a = ds->getGs()();
      const int *ia = ds->getGs().Ip();

      for (int i = 0; i < rFactorSize; i++) {
        double sum = 0;
        for (int j = ia[laInd + i] + 1; j < ia[laInd + i + 1]; j++)
          sum += fabs(a[j]);
        double ai = a[ia[laInd + i]];
        if (ai > sum) {
          rFactorUnsure(i) = 0;
          rFactor(i) = 1.0 / ai;
        }
        else {
          rFactorUnsure(i) = 1;
          rFactor(i) = 1.0 / ai;
        }
      }
    }
  }

  void Joint::checkImpactsForTermination(double dt) {

    const double *a = ds->getGs()();
    const int *ia = ds->getGs().Ip();
    const int *ja = ds->getGs().Jp();
    const Vec &laMBS = ds->getla();
    const Vec &b = ds->getb();

    for (int i = 0; i < forceDir.cols(); i++) {
      gdn(i) = b(laInd + i);
      for (int j = ia[laInd + i]; j < ia[laInd + 1 + i]; j++)
        gdn(i) += a[j] * laMBS(ja[j]);

      if (!fifl->isFulfilled(la(i), gdn(i), gd(i), LaTol, gdTol)) {
        ds->setTermination(false);
        return;
      }
    }
    for (int i = forceDir.cols(); i < forceDir.cols() + momentDir.cols(); i++) {
      gdn(i) = b(laInd + i);
      for (int j = ia[laInd + i]; j < ia[laInd + 1 + i]; j++)
        gdn(i) += a[j] * laMBS(ja[j]);

      if (!fiml->isFulfilled(la(i), gdn(i), gd(i), LaTol, gdTol)) {
        ds->setTermination(false);
        return;
      }
    }
  }

  void Joint::checkConstraintsForTermination() {

    const double *a = ds->getGs()();
    const int *ia = ds->getGs().Ip();
    const int *ja = ds->getGs().Jp();
    const Vec &laMBS = ds->getla();
    const Vec &b = ds->getb();

    for (int i = 0; i < forceDir.cols(); i++) {
      gdd(i) = b(laInd + i);
      for (int j = ia[laInd + i]; j < ia[laInd + 1 + i]; j++)
        gdd(i) += a[j] * laMBS(ja[j]);

      if (!ffl->isFulfilled(la(i), gdd(i), laTol, gddTol)) {
        ds->setTermination(false);
        return;
      }
    }
    for (int i = forceDir.cols(); i < forceDir.cols() + momentDir.cols(); i++) {
      gdd(i) = b(laInd + i);
      for (int j = ia[laInd + i]; j < ia[laInd + 1 + i]; j++)
        gdd(i) += a[j] * laMBS(ja[j]);

      if (!fml->isFulfilled(la(i), gdd(i), laTol, gddTol)) {
        ds->setTermination(false);
        return;
      }
    }
  }

  void Joint::setForceLaw(GeneralizedForceLaw * rc) {
    ffl = rc;
    ffl->setParent(this);
  }

  void Joint::setMomentLaw(GeneralizedForceLaw * rc) {
    fml = rc;
    fml->setParent(this);
  }

  void Joint::setForceDirection(const Mat3xV &fd) {

    forceDir = fd;

    for (int i = 0; i < fd.cols(); i++)
      forceDir.set(i, forceDir.col(i) / nrm2(fd.col(i)));
  }

  void Joint::setMomentDirection(const Mat3xV &md) {

    momentDir = md;

    for (int i = 0; i < md.cols(); i++)
      momentDir.set(i, momentDir.col(i) / nrm2(md.col(i)));
  }

  void Joint::plot(double t, double dt) {
    if (getPlotFeature(plotRecursive) == enabled) {
#ifdef HAVE_OPENMBVCPPINTERFACE
      // WF and WM are needed by OpenMBV plotting in LinkMechanics::plot(...)
      if (isSetValued()) {
        WF[0] = fF[0] * la;
        WF[1] = -WF[0];
        WM[0] = fM[0] * la;
        WM[1] = -WM[0];
      }
#endif
      if (getPlotFeature(generalizedLinkForce) == enabled) {
        for (int j = 0; j < la.size(); j++)
          plotVector.push_back(la(j) / (isSetValued() ? dt : 1.)); // TODO (TS, 2014-09-17) why is there dt in the denominator? does this contradict the link implementation?
      }
      if (getPlotFeature(linkKinematics) == enabled) {
        for (int j = 0; j < g.size(); j++)
          plotVector.push_back(g(j));
        for (int j = 0; j < gd.size(); j++)
          plotVector.push_back(gd(j));
      }
      LinkMechanics::plot(t, dt);
    }
  }

  void Joint::initializeUsingXML(DOMElement *element) {
    LinkMechanics::initializeUsingXML(element);
    DOMElement *e = E(element)->getFirstElementChildNamed(MBSIM%"frameOfReferenceID");
    if (e)
      refFrameID = getInt(e);
    e = E(element)->getFirstElementChildNamed(MBSIM%"forceDirection");
    if (e)
      setForceDirection(getMat(e, 3, 0));
    e = E(element)->getFirstElementChildNamed(MBSIM%"forceLaw");
    if (e)
      setForceLaw(ObjectFactory::createAndInit<GeneralizedForceLaw>(e->getFirstElementChild()));
    e = E(element)->getFirstElementChildNamed(MBSIM%"momentDirection");
    if (e)
      setMomentDirection(getMat(e, 3, 0));
    e = E(element)->getFirstElementChildNamed(MBSIM%"momentLaw");
    if (e)
      setMomentLaw(ObjectFactory::createAndInit<GeneralizedForceLaw>(e->getFirstElementChild()));
    e = E(element)->getFirstElementChildNamed(MBSIM%"connect");
    saved_ref1 = E(e)->getAttribute("ref1");
    saved_ref2 = E(e)->getAttribute("ref2");
#ifdef HAVE_OPENMBVCPPINTERFACE
    e = E(element)->getFirstElementChildNamed(MBSIM%"enableOpenMBVForce");
    if (e) {
      OpenMBVArrow ombv("[-1;1;1]", 0, OpenMBV::Arrow::toHead, OpenMBV::Arrow::toPoint, 1, 1);
      setOpenMBVForce(ombv.createOpenMBV(e));
    }

    e = E(element)->getFirstElementChildNamed(MBSIM%"enableOpenMBVMoment");
    if (e) {
      OpenMBVArrow ombv("[-1;1;1]", 0, OpenMBV::Arrow::toDoubleHead, OpenMBV::Arrow::toPoint, 1, 1);
      setOpenMBVMoment(ombv.createOpenMBV(e));
    }
#endif
  }

  DOMElement* Joint::writeXMLFile(DOMNode *parent) {
    DOMElement *ele0 = LinkMechanics::writeXMLFile(parent);
//    if (forceDir.cols()) {
//      addElementText(ele0, MBSIM%"forceDirection", forceDir);
//      DOMElement *ele1 = new DOMElement(MBSIM%"forceLaw");
//      if (ffl)
//        ffl->writeXMLFile(ele1);
//      ele0->LinkEndChild(ele1);
//    }
//    if (momentDir.cols()) {
//      addElementText(ele0, MBSIM%"momentDirection", momentDir);
//      DOMElement *ele1 = new DOMElement(MBSIM%"momentLaw");
//      if (fml)
//        fml->writeXMLFile(ele1);
//      ele0->LinkEndChild(ele1);
//    }
//    DOMElement *ele1 = new DOMElement(MBSIM%"connect");
//    //ele1->SetAttribute("ref1", frame[0]->getXMLPath(frame[0])); // absolute path
//    //ele1->SetAttribute("ref2", frame[1]->getXMLPath(frame[1])); // absolute path
//    ele1->SetAttribute("ref1", frame[0]->getXMLPath(this, true)); // relative path
//    ele1->SetAttribute("ref2", frame[1]->getXMLPath(this, true)); // relative path
//    ele0->LinkEndChild(ele1);
    return ele0;
  }

  InverseKineticsJoint::InverseKineticsJoint(const string &name) :
      Joint(name), body(0) {
  }

  void InverseKineticsJoint::calcbSize() {
    bSize = body ? body->getuRelSize() : 0;
  }

  void InverseKineticsJoint::updateb(double t) {
    if (bSize) {
      b(Index(0, bSize - 1), Index(0, 2)) = body->getPJT().T();
      b(Index(0, bSize - 1), Index(3, 5)) = body->getPJR().T();
    }
  }

  void InverseKineticsJoint::init(InitStage stage) {
    if (stage == resize) {
      Joint::init(stage);
      x.resize(momentDir.cols());
    }
    else
      Joint::init(stage);
  }

}

