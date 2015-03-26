/* Copyright (C) 2004-2014 MBSim Development Team
 * 
 * This library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 * 
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details. 
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this library; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Contact: martin.o.foerg@googlemail.com
 */

#include <config.h> 
#include "contact.h"

#include <mbsim/contour.h>
#include <mbsim/contour_pdata.h>
#include <mbsim/dynamic_system_solver.h>
#include <mbsim/constitutive_laws.h>
#include <mbsim/contact_kinematics/contact_kinematics.h>
#include <mbsim/utils/contact_utils.h>
#include <fmatvec/function.h>
#include <mbsim/utils/utils.h>
#include <mbsim/objectfactory.h>
#ifdef HAVE_OPENMBVCPPINTERFACE
#include <openmbvcppinterface/group.h>
#include <mbsim/utils/eps.h>
#include <mbsim/utils/rotarymatrices.h>
#endif

#include <algorithm>

using namespace std;
using namespace fmatvec;
using namespace MBXMLUtils;
using namespace xercesc;
using namespace boost;

namespace MBSim {
  extern double tP;
  extern bool gflag;

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(Contact, MBSIM%"Contact")

  Contact::Contact(const string &name) :
      MechanicalLink(name), contacts(0), contactKinematics(0), ckNames(0), plotFeatureMap(), fcl(0), fdf(0), fnil(0), ftil(0)
          , saved_ref(0) {
  }

  Contact::~Contact() {
    for (size_t cK = 0; cK < contactKinematics.size(); cK++) {
      for (int k = 0; k < contactKinematics[cK]->getNumberOfPotentialContactPoints(); ++k)
        delete contactKinematics[cK]->getContactKinematics(k);
      delete contactKinematics[cK];
    }
    delete fcl;
    delete fdf;
    delete fnil;
    delete ftil;
  }

  void Contact::setDynamicSystemSolver(DynamicSystemSolver * sys) {
    ds = sys;
  }

  void Contact::setPlotFeatureContactKinematics(std::string cKName, PlotFeature pf, PlotFeatureStatus value) {
    if (ckNames.end() != find(ckNames.begin(), ckNames.end(), cKName)) {
      pair<string, PlotFeature> Pair(cKName, pf);
      plotFeatureMap.insert(pair<pair<string, PlotFeature>, PlotFeatureStatus>(Pair, value));
    }
  }

#ifdef HAVE_OPENMBVCPPINTERFACE
  shared_ptr<OpenMBV::Group> Contact::getOpenMBVGrp() {
    return openMBVGrp;
  }
#endif

  void Contact::updatewb(double t, int j) {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter)
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updatewb(t, j);
  }

  void Contact::updateW(double t, int j) {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updateW(t, j);
    }
  }

  void Contact::updateV(double t, int j) {
    if (getFrictionDirections()) {
      if (fdf->isSetValued()) {
        for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
          for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
            jter->updateV(t, j);
        }
      }
    }
  }

  void Contact::updateStateDependentVariables(double t) {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter)
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updateKinematics(t);

    if(not(fcl->isSetValued()))
      (*fcl).computeSmoothForces(contacts);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
//        jter->updateNormalForce(t);
        jter->updateTangentialForce(t);
        jter->updateCartesianForces(t);
      }
    }
  }

  void Contact::updateh(double t, int j) {
//   cout << name << " " << j << " t = " << t << endl;
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updateh(t, j);
    }
  }

  void Contact::updateg(double t) {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updateg(t);
    }
  }

  void Contact::updategd(double t) {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updategd(t);
    }
  }

  void Contact::updateStopVector(double t) {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updateStopVector(t);
    }
  }

  void Contact::updateJacobians(double t, int j) {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updateJacobians(t, j);
    }
  }

  void Contact::updateWRef(const Mat& WParent, int j) {
    MechanicalLink::updateWRef(WParent, j);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updateWRef(WParent, j);
    }
  }

  void Contact::updateVRef(const Mat& VParent, int j) {
    MechanicalLink::updateVRef(VParent, j);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updateVRef(VParent, j);
    }
  }

  void Contact::updatehRef(const Vec& hParent, int j) {
    MechanicalLink::updatehRef(hParent, j);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updatehRef(hParent, j);
    }
  }

  void Contact::updatewbRef(const Vec& wbParent) {
    MechanicalLink::updatewbRef(wbParent);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updatewbRef(wbParent);
    }
  }

  void Contact::updatelaRef(const Vec& laParent) {
    MechanicalLink::updatelaRef(laParent);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updatelaRef(laParent);
    }
  }

  void Contact::updategRef(const Vec& gParent) {
    MechanicalLink::updategRef(gParent);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->updategRef(gParent);
      }
    }
  }

  void Contact::updategdRef(const Vec& gdParent) {
    MechanicalLink::updategdRef(gdParent);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updategdRef(gdParent);
    }
  }

  void Contact::updateresRef(const Vec& resParent) {
    MechanicalLink::updateresRef(resParent);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updateresRef(resParent);
    }
  }

  void Contact::updaterFactorRef(const Vec& rFactorParent) {
    MechanicalLink::updaterFactorRef(rFactorParent);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updaterFactorRef(rFactorParent);
    }
  }

  void Contact::updatesvRef(const Vec &svParent) {
    MechanicalLink::updatesvRef(svParent);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updatesvRef(svParent);
    }
  }

  void Contact::updatejsvRef(const VecInt &jsvParent) {
    MechanicalLink::updatejsvRef(jsvParent);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updatejsvRef(jsvParent);
    }
  }

  void Contact::updateLinkStatusRef(const VecInt &LinkStatusParent) {
    MechanicalLink::updateLinkStatusRef(LinkStatusParent);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updateLinkStatusRef(LinkStatusParent);
    }
  }

  void Contact::updateLinkStatusRegRef(const VecInt &LinkStatusRegParent) {
    MechanicalLink::updateLinkStatusRegRef(LinkStatusRegParent);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updateLinkStatusRegRef(LinkStatusRegParent);
    }
  }

  void Contact::calcxSize() {
    MechanicalLink::calcxSize();
    xSize = 0;
  }

  void Contact::calclaSize(int j) {
    MechanicalLink::calclaSize(j);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->calclaSize(j);
        laSize += jter->getlaSize();
      }
    }
  }

  void Contact::calcgSize(int j) {
    MechanicalLink::calcgSize(j);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->calcgSize(j);
        gSize += jter->getgSize();
      }
    }
  }

  void Contact::calcgdSize(int j) {
    MechanicalLink::calcgdSize(j);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->calcgdSize(j);
        gdSize += jter->getgdSize();
      }
    }
  }

  void Contact::calcrFactorSize(int j) {
    MechanicalLink::calcrFactorSize(j);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->calcrFactorSize(j);
        rFactorSize += jter->getrFactorSize();
      }
    }
  }

  void Contact::calcsvSize() {
    MechanicalLink::calcsvSize();
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->calcsvSize();
        svSize += jter->getsvSize();
      }
    }
  }

  void Contact::calcLinkStatusSize() {
    MechanicalLink::calcLinkStatusSize();
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->calcLinkStatusSize();
        LinkStatusSize += jter->getLinkStatusSize();
      }
    }
    LinkStatus.resize(LinkStatusSize);
  }

  void Contact::calcLinkStatusRegSize() {
    MechanicalLink::calcLinkStatusRegSize();
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->calcLinkStatusRegSize();
        LinkStatusRegSize += jter->getLinkStatusRegSize();
      }
    }
    LinkStatusReg.resize(LinkStatusRegSize);
  }

  void Contact::init(InitStage stage) {
    if (stage == resolveXMLPath) {
      //connect all contours given in xml file
      for (size_t i = 0; i < saved_ref.size(); i++) {
        if (saved_ref[i].name1 != "" && saved_ref[i].name2 != "")
          connect(getByPath<Contour>(saved_ref[i].name1), getByPath<Contour>(saved_ref[i].name2), 0, saved_ref[i].contourPairingName);
        //TODO: add option to specifiy contact_kinematics
      }
      if(not(contour.size()))
        THROW_MBSIMERROR("no connection given!");

      //initialize all contour couplings if generalized force law is of maxwell-type
      if (dynamic_cast<MaxwellUnilateralConstraint*>(fcl)) {
        static_cast<MaxwellUnilateralConstraint*>(fcl)->initializeContourCouplings(this);
      }

      MechanicalLink::init(stage);
    }
    else if (stage == preInit) {
      for (size_t cK = 0; cK < contactKinematics.size(); cK++) {
        Contour* contour0 = contour[2 * cK];
        Contour* contour1 = contour[2 * cK + 1];
        contactKinematics[cK]->assignContours(contour0, contour1);

        contacts.push_back(vector<SingleContact>());

        for (int k = 0; k < contactKinematics[cK]->getNumberOfPotentialContactPoints(); ++k) {
          stringstream contactName;
          contactName << ckNames[cK];
          if (contactKinematics[cK]->getNumberOfPotentialContactPoints() > 1)
            contactName << "_" << k;
          contacts[cK].push_back(SingleContact(contactName.str()));
          contacts[cK][k].setContactKinematics(contactKinematics[cK]->getContactKinematics(k) ? contactKinematics[cK]->getContactKinematics(k) : contactKinematics[cK]);
          contacts[cK][k].connect(contour0);
          contacts[cK][k].connect(contour1);
          //Applies the plot feature to all children (make it possible to set only some children...)
          for (int i = plotRecursive; i != LASTPLOTFEATURE; i++) {
            PlotFeature pf = static_cast<PlotFeature>(i);
            PlotFeatureStatus pfS = getPlotFeature(pf);

            pair<string, PlotFeature> Pair(ckNames[cK], pf);
            if (plotFeatureMap.find(Pair) != plotFeatureMap.end()) {
              pfS = plotFeatureMap.find(Pair)->second;
            }

            contacts[cK][k].setPlotFeature(pf, pfS);
          }

          //set the tolerances for the single contacts
          contacts[cK][k].setgTol(gTol);
          contacts[cK][k].setgdTol(gdTol);
          contacts[cK][k].setgddTol(gddTol);
          contacts[cK][k].setlaTol(laTol);
          contacts[cK][k].setLaTol(LaTol);
          contacts[cK][k].setrMax(rMax);
        }
      }

      for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
        for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
          //set parent
          jter->setParent(this);

          //Set dynamic system solver for children
          jter->setDynamicSystemSolver(ds);

          //set contact laws for children
          jter->setNormalForceLaw(fcl);
          jter->setNormalImpactLaw(fnil);
          jter->setTangentialForceLaw(fdf);
          jter->setTangentialImpactLaw(ftil);

#ifdef HAVE_OPENMBVCPPINTERFACE
          //Set OpenMBV-Properties to single contacts
          if (openMBVFrame)
            jter->setOpenMBVContactPoints((iter==contacts.begin() and jter==iter->begin())?openMBVFrame:OpenMBV::ObjectFactory::create(openMBVFrame));
          if (contactArrow)
            jter->setOpenMBVNormalForce((iter==contacts.begin() and jter==iter->begin())?contactArrow:OpenMBV::ObjectFactory::create(contactArrow));
          if (frictionArrow)
            jter->setOpenMBVTangentialForce((iter==contacts.begin() and jter==iter->begin())?frictionArrow:OpenMBV::ObjectFactory::create(frictionArrow));
#endif
          jter->init(stage);
        }
      }
      MechanicalLink::init(stage);
      for(unsigned int i=0; i<contour.size(); i++)
        addDependencies(contour[i]->getDependencies());
    }

    else if (stage == resize) {
      MechanicalLink::init(stage);

      for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
        for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
          jter->init(stage);
      }
    }
    else if (stage == plotting) {
      Element::init(stage);
      updatePlotFeatures();
      if (getPlotFeature(plotRecursive) == enabled) {
#ifdef HAVE_OPENMBVCPPINTERFACE
        openMBVGrp = OpenMBV::ObjectFactory::create<OpenMBV::Group>();
        openMBVGrp->setName(name + "_ContactGroup");
        openMBVGrp->setExpand(false);
        parent->getOpenMBVGrp()->addObject(openMBVGrp);
#endif
        for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
          for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
            jter->init(stage);
        }
      }
    }
    else if (stage == unknownStage) {
      MechanicalLink::init(stage);

      for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
        for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
          jter->init(stage);
      }
    }
    else
      MechanicalLink::init(stage);
    //Don't call init()-routines for "sub"-contacts with stage "LASTINITSTAGE" as here is checked if contactKinematics has more than one possible contact point, which is only possible in multi-contact
    if(fcl) fcl->init(stage);
    if(fdf) fdf->init(stage);
    if(fnil) fnil->init(stage);
    if(ftil) ftil->init(stage);
  }

  bool Contact::isSetValued() const {
    bool flag = fcl->isSetValued();
    if (fdf)
      flag |= fdf->isSetValued();
    return flag;
  }

  bool Contact::isSingleValued() const {
    if (fcl->isSetValued()) {
      if (fdf) {
        return not fdf->isSetValued();
      }
      return false;
    }
    return true;
  }

  void Contact::updateLinkStatus(double dt) {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->updateLinkStatus(dt);
      }
    }
  }

  void Contact::updateLinkStatusReg(double dt) {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->updateLinkStatusReg(dt);
      }
    }
  }

  bool Contact::isActive() const {
    for (std::vector<std::vector<SingleContact> >::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::const_iterator jter = iter->begin(); jter != iter->end(); ++jter)
        if (jter->isActive())
          return true;
    }
    return false;
  }

  bool Contact::gActiveChanged() {
    bool changed = false;
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        if (jter->gActiveChanged())
          changed = true;
    }
    return changed;
  }

  bool Contact::detectImpact() {
    bool impact = false;
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        if (jter->detectImpact())
          impact = true;
    }
    return impact;
  }

  void Contact::plot(double t, double dt) {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->plot(t, dt);
    }
  }

  void Contact::closePlot() {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->closePlot();
    }
    if (getPlotFeature(plotRecursive) == enabled) {
      Element::closePlot();
    }
  }

  void Contact::setNormalForceLaw(GeneralizedForceLaw *fcl_) {
    fcl = fcl_;
    fcl->setParent(this);
  }

  void Contact::setNormalImpactLaw(GeneralizedImpactLaw *fnil_) {
    fnil = fnil_;
    fnil->setParent(this);
  }

  void Contact::setTangentialForceLaw(FrictionForceLaw *fdf_) { 
    fdf = fdf_; 
    fdf->setParent(this);
  }

  void Contact::setTangentialImpactLaw(FrictionImpactLaw *ftil_) {
    ftil = ftil_;
    ftil->setParent(this);
  }

  void Contact::setgInd(int gInd_) {
    MechanicalLink::setgInd(gInd_);
    int nextgInd = gInd_;
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->setgInd(nextgInd);
        nextgInd += jter->getgSize();
      }
    }
  }

  void Contact::setgdInd(int gdInd_) {
    MechanicalLink::setgdInd(gdInd_);
    int nextgdInd = gdInd_;
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->setgdInd(nextgdInd);
        nextgdInd += jter->getgdSize();
      }
    }
  }

  void Contact::setsvInd(int svInd_) {
    MechanicalLink::setsvInd(svInd_);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->setsvInd(svInd_);
        svInd_ += jter->getsvSize();
      }
    }
  }

  void Contact::setlaInd(int laInd_) {
    MechanicalLink::setlaInd(laInd_);
    int nextlaInd = laInd_;
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->setlaInd(nextlaInd);
        nextlaInd += jter->getlaSize();
      }
    }
  }

  void Contact::setrFactorInd(int rFactorInd_) {
    MechanicalLink::setrFactorInd(rFactorInd_);
    int nextrFactorInd = rFactorInd_;
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->setrFactorInd(nextrFactorInd);
        nextrFactorInd += jter->getrFactorSize();
      }
    }
  }

  void Contact::setcorrInd(int corrInd_) {
    MechanicalLink::setcorrInd(corrInd_);
    int nextcorrInd = corrInd_;
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->setcorrInd(nextcorrInd);
        nextcorrInd += jter->getcorrSize();
      }
    }
  }

  void Contact::setLinkStatusInd(int LinkStatusInd_) {
    MechanicalLink::setLinkStatusInd(LinkStatusInd_);
    int nextLinkStatusInd = LinkStatusInd_;
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->setLinkStatusInd(nextLinkStatusInd);
        nextLinkStatusInd += jter->getLinkStatusSize();
      }
    }
  }

  void Contact::setLinkStatusRegInd(int LinkStatusRegInd_) {
    MechanicalLink::setLinkStatusRegInd(LinkStatusRegInd_);
    int nextLinkStatusRegInd = LinkStatusRegInd_;
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->setLinkStatusRegInd(nextLinkStatusRegInd);
        nextLinkStatusRegInd += jter->getLinkStatusRegSize();
      }
    }
  }

  void Contact::solveImpactsFixpointSingle(double dt) {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->solveImpactsFixpointSingle(dt);
    }
  }

  void Contact::solveConstraintsFixpointSingle() {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->solveConstraintsFixpointSingle();
    }
  }

  void Contact::solveImpactsGaussSeidel(double dt) {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->solveImpactsGaussSeidel(dt);
    }
  }

  void Contact::solveConstraintsGaussSeidel() {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->solveConstraintsGaussSeidel();
    }
  }

  void Contact::solveImpactsRootFinding(double dt) {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->solveImpactsRootFinding(dt);
    }
  }

  void Contact::solveConstraintsRootFinding() {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->solveConstraintsRootFinding();
    }
  }

  void Contact::jacobianConstraints() {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->jacobianConstraints();
    }
  }

  void Contact::jacobianImpacts() {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->jacobianImpacts();
    }
  }

  void Contact::updaterFactors() {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updaterFactors();
    }
  }

  void Contact::checkConstraintsForTermination() {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->checkConstraintsForTermination();
    }
  }

  void Contact::checkImpactsForTermination(double dt) {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->checkImpactsForTermination(dt);
    }
  }

  void Contact::checkActive(int j) {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->checkActive(j);
    }
  }

  int Contact::getFrictionDirections() {
    if (fdf)
      return fdf->getFrictionDirections();
    else
      return 0;
  }

  void Contact::connect(Contour *contour0, Contour* contour1, ContactKinematics* contactKinematics_ /*=0*/, const string & name_ /* ="" */) {
    MechanicalLink::connect(contour0);
    MechanicalLink::connect(contour1);
    contactKinematics.push_back(contactKinematics_);

    int cK = contactKinematics.size() - 1;

    if (contactKinematics[cK] == 0)
      contactKinematics[cK] = contour0->findContactPairingWith(contour0->getType(), contour1->getType());
    if (contactKinematics[cK] == 0)
      contactKinematics[cK] = contour1->findContactPairingWith(contour1->getType(), contour0->getType());
    if (contactKinematics[cK] == 0)
      contactKinematics[cK] = contour0->findContactPairingWith(contour1->getType(), contour0->getType());
    if (contactKinematics[cK] == 0)
      contactKinematics[cK] = contour1->findContactPairingWith(contour0->getType(), contour1->getType());
    if (contactKinematics[cK] == 0)
      THROW_MBSIMERROR("(Contact::init): Unknown contact pairing between Contour \"" + contour0->getType() + "\" and Contour \"" + contour1->getType() + "\"!");

    //Create a single contact(with all the information) for every sub contact of each contact kinematics that is part of the multiple contact
    if (name_ == "")
      ckNames.push_back(name + "_" + numtostr(cK));
    else
      ckNames.push_back(name_);

  }

  void Contact::computeCurvatures(Vec & r, int contactKinematicsIndex) const {
    THROW_MBSIMERROR("Not implemented");
    //TODO
  }

  void Contact::LinearImpactEstimation(Vec &gInActive_, Vec &gdInActive_, int *IndInActive_, Vec &gAct_, int *IndActive_) {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->LinearImpactEstimation(gInActive_, gdInActive_, IndInActive_, gAct_, IndActive_);
    }
  }

  void Contact::SizeLinearImpactEstimation(int *sizeInActive_, int *sizeActive_) {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->SizeLinearImpactEstimation(sizeInActive_, sizeActive_);
    }
  }

  void Contact::setlaTol(double tol) {
    MechanicalLink::setlaTol(tol);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->setlaTol(tol);
      }
    }
  }

  void Contact::setLaTol(double tol) {
    MechanicalLink::setLaTol(tol);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->setLaTol(tol);
      }
    }
  }

  void Contact::setgTol(double tol) {
    MechanicalLink::setgTol(tol);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->setgTol(tol);
      }
    }
  }

  void Contact::setgdTol(double tol) {
    MechanicalLink::setgdTol(tol);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->setgdTol(tol);
      }
    }
  }

  void Contact::setgddTol(double tol) {
    MechanicalLink::setgddTol(tol);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->setgddTol(tol);
      }
    }
  }

  void Contact::setrMax(double rMax_) {
    MechanicalLink::setrMax(rMax_);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->setrMax(rMax_);
      }
    }
  }

  void Contact::initializeUsingXML(DOMElement *element) {
    MechanicalLink::initializeUsingXML(element);
    DOMElement *e;

    //Set contact law
    e = E(element)->getFirstElementChildNamed(MBSIM%"normalForceLaw");
    GeneralizedForceLaw *gfl = ObjectFactory::createAndInit<GeneralizedForceLaw>(e->getFirstElementChild());
    setNormalForceLaw(gfl);

    //Get Impact law
    e = E(element)->getFirstElementChildNamed(MBSIM%"normalImpactLaw");
    if (e) {
      GeneralizedImpactLaw *gifl = ObjectFactory::createAndInit<GeneralizedImpactLaw>(e->getFirstElementChild());
      setNormalImpactLaw(gifl);
    }

    //Get Friction Force Law
    e = E(element)->getFirstElementChildNamed(MBSIM%"tangentialForceLaw");
    if (e) {
      FrictionForceLaw *ffl = ObjectFactory::createAndInit<FrictionForceLaw>(e->getFirstElementChild());
      setTangentialForceLaw(ffl);
    }

    //Get Friction Impact Law
    e = E(element)->getFirstElementChildNamed(MBSIM%"tangentialImpactLaw");
    if (e) {
      FrictionImpactLaw *fil = ObjectFactory::createAndInit<FrictionImpactLaw>(e->getFirstElementChild());
      setTangentialImpactLaw(fil);
    }

    /*Read all contour pairings*/
    //Get all contours, that should be connected
    e = E(element)->getFirstElementChildNamed(MBSIM%"connect"); //TODO: all connects must be in a row (is that okay?)
    while (e) { //As long as there are siblings read them and save them
      if (E(e)->getTagName() == MBSIM%"connect") {
        saved_references ref;
        ref.name1 = E(e)->getAttribute("ref1");
        ref.name2 = E(e)->getAttribute("ref2");
        if (E(e)->hasAttribute("name"))
          ref.contourPairingName = E(e)->getAttribute("name");
        else
          ref.contourPairingName = "";
        //TODO: add possibility of defining own contactKinematics? (also in Contact-class)

        saved_ref.push_back(ref);
        e = e->getNextElementSibling();
      }
      else {
        break;
      }
    }

#ifdef HAVE_OPENMBVCPPINTERFACE
    //Get all drawing thingies
    if (E(element)->getFirstElementChildNamed(MBSIM%"enableOpenMBVContactPoints")) {
      OpenMBVFrame ombv;
      openMBVFrame = ombv.createOpenMBV(e);
    }

    e = E(element)->getFirstElementChildNamed(MBSIM%"enableOpenMBVNormalForce");
    if (e) {
      OpenMBVArrow ombv("[-1;1;1]", 0, OpenMBV::Arrow::toHead, OpenMBV::Arrow::toPoint, 1, 1);
      contactArrow = ombv.createOpenMBV(e);
    }

    e = E(element)->getFirstElementChildNamed(MBSIM%"enableOpenMBVTangentialForce");
    if (e) {
      OpenMBVArrow ombv("[-1;1;1]", 0, OpenMBV::Arrow::toHead, OpenMBV::Arrow::toPoint, 1, 1);
      frictionArrow = ombv.createOpenMBV(e);
    }
#endif
  }

  DOMElement* Contact::writeXMLFile(DOMNode *parent) {
    DOMElement *ele0 = MechanicalLink::writeXMLFile(parent);
//    DOMElement *ele1;
//    ele1 = new DOMElement(MBSIM%"normalForceLaw");
//    if (fcl)
//      fcl->writeXMLFile(ele1);
//    ele0->LinkEndChild(ele1);
//    if (fnil) {
//      ele1 = new DOMElement(MBSIM%"normalImpactLaw");
//      fnil->writeXMLFile(ele1);
//      ele0->LinkEndChild(ele1);
//    }
//    if (fdf) {
//      ele1 = new DOMElement(MBSIM%"tangentialForceLaw");
//      fdf->writeXMLFile(ele1);
//      ele0->LinkEndChild(ele1);
//    }
//    if (ftil) {
//      ele1 = new DOMElement(MBSIM%"tangentialImpactLaw");
//      ftil->writeXMLFile(ele1);
//      ele0->LinkEndChild(ele1);
//    }
//    ele1 = new DOMElement(MBSIM%"connect");
//    //for(unsigned int i=0; i<saved_ref.size(); i++) {
//    ele1->SetAttribute("ref1", contour[0]->getXMLPath(this, true)); // relative path
//    ele1->SetAttribute("ref2", contour[1]->getXMLPath(this, true)); // relative path
//    //}
//    ele0->LinkEndChild(ele1);
//#ifdef HAVE_OPENMBVCPPINTERFACE
////    if(openMBVContactFrameSize>0)
////      addElementText(ele0,MBSIM%"enableOpenMBVContactPoints",openMBVContactFrameSize);
//    if (contactArrow) {
//      ele1 = new DOMElement(MBSIM%"openMBVNormalForceArrow");
//      contactArrow->writeXMLFile(ele1);
//      ele0->LinkEndChild(ele1);
//    }
//    if (frictionArrow) {
//      ele1 = new DOMElement(MBSIM%"openMBVTangentialForceArrow");
//      frictionArrow->writeXMLFile(ele1);
//      ele0->LinkEndChild(ele1);
//    }
//#endif
    return ele0;
  }

  void Contact::updatecorrRef(const Vec& corrParent) {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updatecorrRef(corrParent);
    }
  }

  void Contact::updatecorr(int j) {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->updatecorr(j);
    }
  }

  void Contact::calccorrSize(int j) {
    MechanicalLink::calccorrSize(j);
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
        jter->calccorrSize(j);
        corrSize += jter->getcorrSize();
      }
    }
  }

  void Contact::checkRoot() {
    for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
      for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter)
        jter->checkRoot();
    }
  }
}

