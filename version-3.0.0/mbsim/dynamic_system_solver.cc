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
 * Contact: mfoerg@users.berlios.de
 *          rzander@users.berlios.de
 */

#include<config.h>
#include "mbsim/dynamic_system_solver.h"
#include "mbsim/modelling_interface.h"
#include "mbsim/frame.h"
#include "mbsim/contour.h"
#include "mbsim/link.h"
#include "mbsim/graph.h"
#include "mbsim/extra_dynamic.h"
#include "mbsim/integrators/integrator.h"
#include "mbsim/utils/eps.h"
#include "dirent.h"
#include <mbsim/environment.h>
#include <mbsim/objectfactory.h>

#ifndef MINGW
#  include<sys/stat.h>
#else
#  include<io.h>
#  define mkdir(a,b) mkdir(a)
#endif

#include <H5Cpp.h>
#include <hdf5serie/fileserie.h>
#include <hdf5serie/simpleattribute.h>
#include <hdf5serie/simpledataset.h>

#ifdef HAVE_ANSICSIGNAL
#  include <signal.h>
#endif

#ifdef HAVE_OPENMBVCPPINTERFACE
#include "openmbvcppinterface/group.h"
#endif

//#ifdef _OPENMP
//#include <omp.h>
//#endif

using namespace std;
using namespace fmatvec;

namespace MBSim {

  bool DynamicSystemSolver::exitRequest=false;

  DynamicSystemSolver::DynamicSystemSolver() : Group("Default"), maxIter(10000), highIter(1000), maxDampingSteps(3), lmParm(0.001), contactSolver(FixedPointSingle), impactSolver(FixedPointSingle), strategy(local), linAlg(LUDecomposition), stopIfNoConvergence(false), dropContactInfo(false), useOldla(true), numJac(false), checkGSize(true), limitGSize(500), warnLevel(0), peds(false), impact(false), sticking(false), driftCount(1), flushEvery(100000), flushCount(flushEvery), reorganizeHierarchy(true), tolProj(1e-16), alwaysConsiderContact(true), INFO(true), READZ0(false), truncateSimulationFiles(true) { 
    constructor();
  } 

  DynamicSystemSolver::DynamicSystemSolver(const string &projectName) : Group(projectName), maxIter(10000), highIter(1000), maxDampingSteps(3), lmParm(0.001), contactSolver(FixedPointSingle), impactSolver(FixedPointSingle), strategy(local), linAlg(LUDecomposition), stopIfNoConvergence(false), dropContactInfo(false), useOldla(true), numJac(false), checkGSize(true), limitGSize(500), warnLevel(0), peds(false), impact(false), sticking(false), driftCount(1), flushEvery(100000), flushCount(flushEvery), reorganizeHierarchy(true), tolProj(1e-16), alwaysConsiderContact(true), INFO(true), READZ0(false), truncateSimulationFiles(true) { 
    constructor();
  }

  DynamicSystemSolver::~DynamicSystemSolver() { 
    H5::FileSerie::deletePIDFiles();
  }

  void DynamicSystemSolver::initialize() {
//#ifdef _OPENMP
//    omp_set_nested(true);
//#endif
#ifdef HAVE_ANSICSIGNAL
    signal(SIGINT, sigInterruptHandler);
    signal(SIGTERM, sigInterruptHandler);
    signal(SIGABRT, sigAbortHandler);
#endif
    for(int stage=0; stage<MBSim::LASTINITSTAGE; stage++) {
      if(INFO) cout<<"Initializing stage "<<stage<<"/"<<LASTINITSTAGE-1<<endl;
      init((InitStage)stage);
      if(INFO) cout<<"Done initializing stage "<<stage<<"/"<<LASTINITSTAGE-1<<endl;
    }
  }

  void DynamicSystemSolver::init(InitStage stage) {
    if(stage==MBSim::reorganizeHierarchy && reorganizeHierarchy) {
      if(INFO) cout <<name << " (special group) stage==preInit:" << endl;

      vector<Object*> objList;
      buildListOfObjects(objList,true);

      vector<Frame*> frmList;
      buildListOfFrames(frmList,true);

      vector<Contour*> cntList;
      buildListOfContours(cntList,true);

      vector<Link*> lnkList;
      buildListOfLinks(lnkList,true);

      vector<ExtraDynamic*> edList;
      buildListOfExtraDynamic(edList,true);

      vector<ModellingInterface*> modellList;
      buildListOfModels(modellList,true);

      if(modellList.size())
        do {
          modellList[0]->processModellList(modellList,objList,lnkList);
        } while(modellList.size());

      dynamicsystem.clear(); // delete old DynamicSystem list
      object.clear(); // delete old object list
      frame.clear(); // delete old frame list
      contour.clear(); // delete old contour list
      link.clear(); // delete old link list
      extraDynamic.clear(); // delete old ed list

      /* rename system structure */
      if(INFO) cout << "object List:" << endl;
      for(unsigned int i=0; i<objList.size(); i++) {
        stringstream str;
        str << objList[i]->getPath('/');
        if(INFO) cout<<str.str()<<endl;
        objList[i]->setName(str.str());
      }
      if(INFO) cout << "frame List:" << endl;
      for(unsigned int i=0; i<frmList.size(); i++) {
        stringstream str;
        str << frmList[i]->getParent()->getPath('/') << "/" << frmList[i]->getName();
        if(INFO) cout<<str.str()<<endl;
        frmList[i]->setName(str.str());
        addFrame(frmList[i]);
      }
      if(INFO) cout << "contour List:" << endl;
      for(unsigned int i=0; i<cntList.size(); i++) {
        stringstream str;
        str << cntList[i]->getParent()->getPath('/') << "/" << cntList[i]->getName();
        if(INFO) cout<<str.str()<<endl;
        cntList[i]->setName(str.str());
        addContour(cntList[i]);
      }
      if(INFO) cout << "link List:" << endl;
      for(unsigned int i=0; i<lnkList.size(); i++) {
        stringstream str;
        str << lnkList[i]->getParent()->getPath('/') << "/" << lnkList[i]->getName();
        if(INFO) cout<<str.str()<<endl;
        lnkList[i]->setName(str.str());
        addLink(lnkList[i]);
      }
      if(INFO) cout << "ed List:" << endl;
      for(unsigned int i=0; i<edList.size(); i++) {
        stringstream str;
        str << edList[i]->getParent()->getPath('/') << "/" << edList[i]->getName();
        if(INFO) cout<<str.str()<<endl;
        edList[i]->setName(str.str());
        addExtraDynamic(edList[i]);
      }

      /* matrix of body dependencies */
      SqrMat A(objList.size(),INIT,0.);
      for(unsigned int i=0; i<objList.size(); i++) {

        vector<Object*> parentBody = objList[i]->getObjectsDependingOn();

        for(unsigned int h=0; h<parentBody.size(); h++) {
          unsigned int j=0;
          bool foundBody = false;
          for(unsigned int k=0; k<objList.size(); k++, j++) {
            if(objList[k] == parentBody[h]) {
              foundBody = true;
              break;
            }
          }

          if(foundBody) {
            A(i,j) = 2; // 2 means predecessor
            A(j,i) = 1; // 1 means successor
          }
        }
      }
      // if(INFO) cout << "A = " << A << endl;

      vector<Graph*> bufGraph;
      int nt = 0;
      for(int i=0; i<A.size(); i++) {
        double a = max(A.T().col(i));
        if(a>0 && fabs(A(i,i)+1)>epsroot() ) { // root of relativ kinematics
          stringstream str;
          str << "InvisibleGraph" << nt++;
          Graph *graph = new Graph(str.str());
          addToGraph(graph, A, i, objList);
          graph->setPlotFeatureRecursive(plotRecursive, enabled); // the generated invisible graph must always walk through the plot functions
          bufGraph.push_back(graph);
        } 
        else if(fabs(a)<epsroot()) // absolut kinematics
          addObject(objList[i]);
      }
      // if(INFO) cout << "A = " << A << endl;

      for(unsigned int i=0; i<bufGraph.size(); i++) {
        addGroup(bufGraph[i]);
      }

      if(INFO) cout << "End of special group stage==preInit" << endl;

      // after reorganizing a resize is required
      init(resize);
    }
    else if(stage==resize) {
      calcqSize();
      calcuSize(0);
      calcuSize(1);
      sethSize(uSize[0]);
      sethSize(uSize[1],1);
      setUpLinks(); // is needed by calcgSize()

      calcxSize();

      calclaInverseKineticsSize(); 
      calcbInverseKineticsSize(); 

      calclaSize();
      calcgSize();
      calcgdSize();
      calcrFactorSize();
      calcsvSize();
      svSize += 1; // TODO additional event for drift 
      calcLinkStatusSize();      

      if(INFO) cout << "qSize = " << qSize << endl;
      if(INFO) cout << "uSize[0] = " << uSize[0] << endl;
      if(INFO) cout << "xSize = " << xSize << endl;
      if(INFO) cout << "gSize = " << gSize << endl;
      if(INFO) cout << "gdSize = " << gdSize << endl;
      if(INFO) cout << "laSize = " << laSize << endl;
      if(INFO) cout << "svSize = " << svSize << endl;
      if(INFO) cout << "LinkStatusSize = " << LinkStatusSize <<endl;
      if(INFO) cout << "hSize[0] = " << hSize[0] << endl;

      if(INFO) cout << "uSize[1] = " << uSize[1] << endl;
      if(INFO) cout << "hSize[1] = " << hSize[1] << endl;

      Group::init(stage);
    }
    else if(stage==unknownStage) {
      setDynamicSystemSolver(this);

      // TODO memory problem with many contacts
      if(laSize>8000)
        laSize=8000;
      MParent[0].resize(getuSize(0));
      MParent[1].resize(getuSize(1));
      TParent.resize(getqSize(),getuSize());
      LLMParent[0].resize(getuSize(0));
      LLMParent[1].resize(getuSize(1));
      WParent[0].resize(getuSize(0),getlaSize());
      VParent[0].resize(getuSize(0),getlaSize());
      WParent[1].resize(getuSize(1),getlaSize());
      VParent[1].resize(getuSize(1),getlaSize());
      cout << WParent[0] << endl;
      cout << WParent[1] << endl;
      wbParent.resize(getlaSize());
      laParent.resize(getlaSize());
      rFactorParent.resize(getlaSize());
      sParent.resize(getlaSize());
      if(impactSolver==RootFinding) resParent.resize(getlaSize());
      gParent.resize(getgSize());
      gdParent.resize(getgdSize());
      zdParent.resize(getzSize());
      udParent1.resize(getuSize(1));
      hParent[0].resize(getuSize(0));
      hParent[1].resize(getuSize(1));
      dhdqObjectParent.resize(getuSize(1),getqSize());
      dhdqLinkParent.resize(getuSize(1),getqSize());
      dhduObjectParent.resize(getuSize(1));
      dhduLinkParent.resize(getuSize(1));
      dhdtObjectParent.resize(getuSize(1));
      dhdtLinkParent.resize(getuSize(1));
      rParent[0].resize(getuSize(0));
      rParent[1].resize(getuSize(1));
      fParent.resize(getxSize());
      svParent.resize(getsvSize());
      jsvParent.resize(getsvSize());
      LinkStatusParent.resize(getLinkStatusSize());
      WInverseKineticsParent[0].resize(hSize[0],laInverseKineticsSize);
      WInverseKineticsParent[1].resize(hSize[1],laInverseKineticsSize);
      bInverseKineticsParent.resize(bInverseKineticsSize,laInverseKineticsSize);
      laInverseKineticsParent.resize(laInverseKineticsSize);

      updateMRef(MParent[0],0);
      updateMRef(MParent[1],1);
      updateTRef(TParent);
      updateLLMRef(LLMParent[0],0);
      updateLLMRef(LLMParent[1],1);

      Group::init(stage);

      updatesvRef(svParent);
      updatejsvRef(jsvParent);
      updateLinkStatusRef(LinkStatusParent);
      updatezdRef(zdParent);
      updateudRef(udParent1,1);
      updateudallRef(udParent1,1);
      updatelaRef(laParent);
      updategRef(gParent);
      updategdRef(gdParent);
      updatehRef(hParent[0],0);
      updaterRef(rParent[0],0);
      updatehRef(hParent[1],1);
      updaterRef(rParent[1],1);
      updateWRef(WParent[0],0);
      updateWRef(WParent[1],1);
      updateVRef(VParent[0],0);
      updateVRef(VParent[1],1);
      updatewbRef(wbParent);

      updatelaInverseKineticsRef(laInverseKineticsParent);
      updateWInverseKineticsRef(WInverseKineticsParent[0],0);
      updateWInverseKineticsRef(WInverseKineticsParent[1],1);
      updatebInverseKineticsRef(bInverseKineticsParent);
      updatehRef(hParent[1],1);
      updaterRef(rParent[1],1);
      updateWRef(WParent[1],1);
      updateVRef(VParent[1],1);

      if(impactSolver==RootFinding) updateresRef(resParent);
      updaterFactorRef(rFactorParent);

      // contact solver specific settings
      if(INFO) cout << "  use contact solver \'" << getSolverInfo() << "\' for contact situations" << endl;
      if(contactSolver == GaussSeidel) solveConstraints_ = &DynamicSystemSolver::solveConstraintsGaussSeidel; 
      else if(contactSolver == LinearEquations) {
        solveConstraints_ = &DynamicSystemSolver::solveConstraintsLinearEquations;
        cout << "WARNING: solveLL is only valid for bilateral constrained systems!" << endl;
      }
      else if(contactSolver == FixedPointSingle) solveConstraints_ = &DynamicSystemSolver::solveConstraintsFixpointSingle;
      else if(contactSolver == RootFinding) { cout << "WARNING: RootFinding solver is BUGGY at least if there is friction!" << endl; solveConstraints_ = &DynamicSystemSolver::solveConstraintsRootFinding; }
      else throw MBSimError("ERROR (DynamicSystemSolver::init()): Unknown contact solver");

      // impact solver specific settings
      if(INFO) cout << "  use impact solver \'" << getSolverInfo() << "\' for impact situations" << endl;
      if(impactSolver == GaussSeidel) solveImpacts_ = &DynamicSystemSolver::solveImpactsGaussSeidel; 
      else if(impactSolver == LinearEquations) {
        solveImpacts_ = &DynamicSystemSolver::solveImpactsLinearEquations;
        cout << "WARNING: solveLL is only valid for bilateral constrained systems!" << endl;
      }
      else if(impactSolver == FixedPointSingle) solveImpacts_ = &DynamicSystemSolver::solveImpactsFixpointSingle;
      else if(impactSolver == RootFinding) { cout << "WARNING: RootFinding solver is BUGGY at least if there is friction!" << endl; solveImpacts_ = &DynamicSystemSolver::solveImpactsRootFinding; }
      else throw MBSimError("ERROR (DynamicSystemSolver::init()): Unknown impact solver");
    }
    else if(stage==MBSim::modelBuildup) {
      if(INFO) cout << "  initialising modelBuildup ..." << endl;
      Group::init(stage);
      setDynamicSystemSolver(this);
      checkForConstraints(); // TODO for preinit
    }
    else if(stage==MBSim::plot) {
      if(INFO) cout << "  initialising plot-files ..." << endl;
      Group::init(stage);
#ifdef HAVE_OPENMBVCPPINTERFACE
      if(getPlotFeature(plotRecursive)==enabled) {
        openMBVGrp->writeXML();
        if(truncateSimulationFiles) openMBVGrp->writeH5();
      }
#endif
      if(INFO) cout << "...... done initialising." << endl << endl;
    }
    else if(stage==MBSim::calculateLocalInitialValues) {
      Group::initz();
      updateStateDependentVariables(0);
      updateg(0);
      Group::init(stage);
    }
    else
      Group::init(stage);
  }

  int DynamicSystemSolver::solveConstraintsFixpointSingle() {
    updaterFactors();

    checkConstraintsForTermination();
    if(term) return 0;

    int iter, level = 0;
    int checkTermLevel = 0;

    for(iter = 1; iter<=maxIter; iter++) {

      if(level < decreaseLevels.size() && iter > decreaseLevels(level)) {
        level++;
        decreaserFactors();
        cout << endl << "WARNING: decreasing r-factors at iter = " << iter << endl;
        if(warnLevel>=2) cout <<endl<< "WARNING: decreasing r-factors at iter = " << iter << endl;
      }

      Group::solveConstraintsFixpointSingle();

      if(checkTermLevel >= checkTermLevels.size() || iter > checkTermLevels(checkTermLevel)) {
        checkTermLevel++;
        checkConstraintsForTermination();
        if(term) break;
      }
    }
    return iter;
  }

  int DynamicSystemSolver::solveImpactsFixpointSingle(double dt) {
    updaterFactors();

    checkImpactsForTermination(dt);
    if(term) return 0;

    int iter, level = 0;
    int checkTermLevel = 0;

    for(iter = 1; iter<=maxIter; iter++) {

      if(level < decreaseLevels.size() && iter > decreaseLevels(level)) {
        level++;
        decreaserFactors();
        cout << endl << "WARNING: decreasing r-factors at iter = " << iter << endl;
        if(warnLevel>=2) cout << endl << "WARNING: decreasing r-factors at iter = " << iter << endl;
      }

      Group::solveImpactsFixpointSingle(dt);

      if(checkTermLevel >= checkTermLevels.size() || iter > checkTermLevels(checkTermLevel)) {
        checkTermLevel++;
        checkImpactsForTermination(dt);
        if(term) break;
      }
    }
    return iter;
  }

  int DynamicSystemSolver::solveConstraintsGaussSeidel() {
    checkConstraintsForTermination();
    if(term) return 0 ;

    int iter;
    int checkTermLevel = 0;

    for(iter = 1; iter<=maxIter; iter++) {
      Group::solveConstraintsGaussSeidel();
      if(checkTermLevel >= checkTermLevels.size() || iter > checkTermLevels(checkTermLevel)) {
        checkTermLevel++;
        checkConstraintsForTermination();
        if(term) break;
      }
    }
    return iter;
  }

  int DynamicSystemSolver::solveImpactsGaussSeidel(double dt) {
    checkImpactsForTermination(dt);
    if(term) return 0 ;

    int iter;
    int checkTermLevel = 0;

    for(iter = 1; iter<=maxIter; iter++) {
      Group::solveImpactsGaussSeidel(dt);
      if(checkTermLevel >= checkTermLevels.size() || iter > checkTermLevels(checkTermLevel)) {
        checkTermLevel++;
        checkImpactsForTermination(dt);
        if(term) break;
      }
    }
    return iter;
  }

  int DynamicSystemSolver::solveConstraintsRootFinding() {
    updaterFactors();

    int iter;
    int checkTermLevel = 0;

    updateresRef(resParent(0,laSize-1));
    Group::solveConstraintsRootFinding(); 

    double nrmf0 = nrm2(res);
    Vec res0 = res.copy();

    checkConstraintsForTermination();
    if(term)
      return 0 ;

    DiagMat I(la.size(),INIT,1);
    for(iter=1; iter<maxIter; iter++) {

      if(Jprox.size() != la.size()) Jprox.resize(la.size(),NONINIT);

      if(numJac) {
        for(int j=0; j<la.size(); j++) {
          const double xj = la(j);
          double dx = epsroot()/2.;
          
          do dx += dx;
          while (fabs(xj + dx - la(j)) < epsroot());

          la(j) += dx;
          Group::solveConstraintsRootFinding(); 
          la(j) = xj;
          Jprox.col(j) = (res-res0)/dx;
        }
      } 
      else jacobianConstraints();
      Vec dx;
      if(linAlg == LUDecomposition) dx >> slvLU(Jprox,res0);
      else if(linAlg == LevenbergMarquardt) {
        SymMat J = SymMat(JTJ(Jprox) + lmParm*I);
        dx >> slvLL(J,Jprox.T()*res0);
      }
      else if(linAlg == PseudoInverse) dx >> slvLS(Jprox,res0);
      else throw 5;

      Vec La_old = la.copy();
      double alpha = 1;       
      double nrmf = 1;
      for (int k=0; k<maxDampingSteps; k++) {
        la = La_old - alpha*dx;
        solveConstraintsRootFinding();
        nrmf = nrm2(res);
        if(nrmf < nrmf0) break;

        alpha *= .5;
      }
      nrmf0 = nrmf;
      res0 = res;

      if(checkTermLevel >= checkTermLevels.size() || iter > checkTermLevels(checkTermLevel)) {
        checkTermLevel++;
        checkConstraintsForTermination();
        if(term) break;
      }
    }
    return iter;
  }

  int DynamicSystemSolver::solveImpactsRootFinding(double dt) {
    updaterFactors();

    int iter;
    int checkTermLevel = 0;

    updateresRef(resParent(0,laSize-1));
    Group::solveImpactsRootFinding(dt); 
    
    double nrmf0 = nrm2(res);
    Vec res0 = res.copy();

    checkImpactsForTermination(dt);
    if(term)
      return 0 ;

    DiagMat I(la.size(),INIT,1);
    for(iter=1; iter<maxIter; iter++) {

      if(Jprox.size() != la.size()) Jprox.resize(la.size(),NONINIT);

      if(numJac) {
        for(int j=0; j<la.size(); j++) {
          const double xj = la(j);
          double dx = .5*epsroot();
          
          do dx += dx;
          while (fabs(xj + dx - la(j)) < epsroot());
          
          la(j) += dx;
          Group::solveImpactsRootFinding(dt); 
          la(j) = xj;
          Jprox.col(j) = (res-res0)/dx;
        }
      }
      else jacobianImpacts();
      Vec dx;
      if(linAlg == LUDecomposition) dx >> slvLU(Jprox,res0);
      else if(linAlg == LevenbergMarquardt) {
        SymMat J = SymMat(JTJ(Jprox) + lmParm*I);
        dx >> slvLL(J,Jprox.T()*res0);
      }
      else if(linAlg == PseudoInverse) dx >> slvLS(Jprox,res0);
      else throw 5;

      Vec La_old = la.copy();
      double alpha = 1.;
      double nrmf = 1;
      for (int k=0; k<maxDampingSteps; k++) {
        la = La_old - alpha*dx;
        solveImpactsRootFinding(dt);
        nrmf = nrm2(res);
        if(nrmf < nrmf0) break;
        alpha *= .5;  
      }
      nrmf0 = nrmf;
      res0 = res;

      if(checkTermLevel >= checkTermLevels.size() || iter > checkTermLevels(checkTermLevel)) {
        checkTermLevel++;
        checkImpactsForTermination(dt);
        if(term) break;
      }
    }
    return iter;
  }

  void DynamicSystemSolver::checkConstraintsForTermination() {
    term = true;
    for(vector<DynamicSystem*>::iterator i = dynamicsystem.begin(); i != dynamicsystem.end(); ++i) { 
      (*i)->checkConstraintsForTermination(); 
      if(term == false) return;
    }

    for(vector<Link*>::iterator i = linkSetValuedActive.begin(); i != linkSetValuedActive.end(); ++i) {
      (**i).checkConstraintsForTermination();
      if(term == false) return;
    }
  }

  void DynamicSystemSolver::checkImpactsForTermination(double dt) {
    term = true;
    for(vector<DynamicSystem*>::iterator i = dynamicsystem.begin(); i != dynamicsystem.end(); ++i) {
      (*i)->checkImpactsForTermination(dt); 
      if(term == false) return;
    }

    for(vector<Link*>::iterator i = linkSetValuedActive.begin(); i != linkSetValuedActive.end(); ++i) {
      (**i).checkImpactsForTermination(dt);
      if(term == false) return;
    }
  }

  void DynamicSystemSolver::updateh(double t, int j) {
    h[j].init(0);
    Group::updateh(t,j);
  }

  void DynamicSystemSolver::updateh0Fromh1(double t) {
    h[0].init(0);
    Group::updateh0Fromh1(t);
  }

  void DynamicSystemSolver::updateW0FromW1(double t) {
    W[0].init(0);
    Group::updateW0FromW1(t);
  }

  void DynamicSystemSolver::updateV0FromV1(double t) {
    V[0].init(0);
    Group::updateV0FromV1(t);
  }

  void DynamicSystemSolver::updatedhdz(double t) {
    h[0].init(0);
    dhdqObject.init(0);
    dhdqLink.init(0);
    dhduObject.init(0);
    dhduLink.init(0);
    dhdtObject.init(0);
    dhdtLink.init(0);
    Group::updatedhdz(t);
  }

  void DynamicSystemSolver::updateM(double t, int i) {
    M[i].init(0);
    Group::updateM(t,i);
  }

  void DynamicSystemSolver::updateStateDependentVariables(double t) {
    Group::updateStateDependentVariables(t);

    if(integratorExitRequest) { // if the integrator has not exit after a integratorExitRequest
      cout<<"MBSim: Integrator has not stopped integration! Terminate NOW the hard way!"<<endl;
      H5::FileSerie::deletePIDFiles();
      _exit(1);
    }

    if(exitRequest) { // on exitRequest flush plot files and ask the integrator to exit
      cout<<"MBSim: Flushing HDF5 files and ask integrator to terminate!"<<endl;
      H5::FileSerie::flushAllFiles();
      integratorExitRequest=true;
    }

    if(H5::FileSerie::getFlushOnes()) H5::FileSerie::flushAllFiles(); // flush files ones if requested
  }

  void DynamicSystemSolver::updater(double t, int j) {
    r[j] = V[j]*la; // cannot be called locally (hierarchically), because this adds some values twice to r for tree structures
  }

  void DynamicSystemSolver::updatewb(double t, int j) {
    wb.init(0);
    Group::updatewb(t,j);
  }

  void DynamicSystemSolver::updateW(double t, int j) {
    W[j].init(0);
    Group::updateW(t,j);
  }

  void DynamicSystemSolver::updateV(double t, int j) {
    V[j] = W[j];
    Group::updateV(t,j);
  }

  void DynamicSystemSolver::closePlot() {
    if(getPlotFeature(plotRecursive)==enabled) {
      Group::closePlot();
    }
  }

  int DynamicSystemSolver::solveConstraints() {
    if(la.size()==0) return 0;

    if(useOldla)initla();
    else la.init(0);

    int iter;
    Vec laOld;
    laOld = la;
    iter = (this->*solveConstraints_)(); // solver election
    if(iter >= maxIter) {
      if(INFO)  { 
        cout << endl;
        cout << "Iterations: " << iter << endl;
        cout << "\nError: no convergence."<< endl;
      }
      if(stopIfNoConvergence) {
        if(dropContactInfo) dropContactMatrices();
        assert(iter < maxIter);
      }
      if(INFO) cout << "Anyway, continuing integration..."<< endl;
    }

    if(warnLevel>=1 && iter>highIter)
      cerr <<endl<< "Warning: high number of iterations: " << iter << endl;

    if(useOldla) savela();

    return iter;
  }

  int DynamicSystemSolver::solveImpacts(double dt) {
    if(la.size()==0) return 0;
    double H=1;
    if (dt>0) H=dt;
    
    if(useOldla)initla(H);
    else la.init(0);

    int iter;
    Vec laOld;
    laOld = la;
    iter = (this->*solveImpacts_)(dt); // solver election
    if(iter >= maxIter) {
      if (INFO) {
        cout << endl;
        cout << "Iterations: " << iter << endl;
        cout << "\nError: no convergence."<< endl;
      }
      if(stopIfNoConvergence) {
        if(dropContactInfo) dropContactMatrices();
        assert(iter < maxIter);
      }
      if (INFO) cout << "Anyway, continuing integration..."<< endl;
    }

    if(warnLevel>=1 && iter>highIter)
      cerr <<endl<< "Warning: high number of iterations: " << iter << endl;

    if(useOldla) savela(H);

    return iter;
  }

  void DynamicSystemSolver::computeInitialCondition() {
    updateStateDependentVariables(0);
    updateg(0);
    checkActiveg();
    checkActiveLinks();
    updategd(0);
    checkActivegd();
    checkState();
    updateJacobians(0);
    calclaSize();
    calcrFactorSize();
    updateWRef(WParent[0](Index(0,getuSize()-1),Index(0,getlaSize()-1)),0);
    updateVRef(VParent[0](Index(0,getuSize()-1),Index(0,getlaSize()-1)),0);
    updatelaRef(laParent(0,laSize-1));
    updatewbRef(wbParent(0,laSize-1));
    updaterFactorRef(rFactorParent(0,rFactorSize-1));
  }

  Vec DynamicSystemSolver::deltau(const Vec &zParent, double t, double dt) {
    if(q()!=zParent()) updatezRef(zParent);

    updater(t); // TODO update should be outside
    updatedu(t,dt);
    return ud[0];
  }

  Vec DynamicSystemSolver::deltaq(const Vec &zParent, double t, double dt) {
    if(q()!=zParent()) updatezRef(zParent);
    updatedq(t,dt);

    return qd;
  }

  Vec DynamicSystemSolver::deltax(const Vec &zParent, double t, double dt) {
    if(q()!=zParent()) {
      updatezRef(zParent);
    }
    updatedx(t,dt);
    return xd;
  }

  void DynamicSystemSolver::initz(Vec& z) {
    updatezRef(z);
    if(READZ0) {
      q = q0;
      u = u0;
      x = x0;
    }
    else Group::initz();
  }

  int DynamicSystemSolver::solveConstraintsLinearEquations() {
    la = slvLS(G,-(W[0].T()*slvLLFac(LLM[0],h[0]) + wb));
    return 1;
  }

  int DynamicSystemSolver::solveImpactsLinearEquations(double dt) {
    la = slvLS(G,-(gd + W[0].T()*slvLLFac(LLM[0],h[0])*dt));
    return 1;
  }

  void DynamicSystemSolver::updateG(double t, int j) {
    G.resize() = SqrMat(W[j].T()*slvLLFac(LLM[j],V[j])); 

    if(checkGSize) Gs.resize();
    else if(Gs.cols() != G.size()) {
      static double facSizeGs = 1;
      if(G.size()>limitGSize && fabs(facSizeGs-1) < epsroot()) facSizeGs = double(countElements(G))/double(G.size()*G.size())*1.5;
      Gs.resize(G.size(),int(G.size()*G.size()*facSizeGs));
    }
    Gs << G;
  }

  void DynamicSystemSolver::decreaserFactors() {

    for(vector<Link*>::iterator i = linkSetValuedActive.begin(); i != linkSetValuedActive.end(); ++i)
      (*i)->decreaserFactors();
  }

  void DynamicSystemSolver::update(const Vec &zParent, double t, int options) {
    if(q()!=zParent()) updatezRef(zParent);

    updateStateDependentVariables(t);
    updateg(t);
    checkActiveg();
    checkActiveLinks();
    if(gActiveChanged() || options==1 ) {

      // checkAllgd(); // TODO necessary?
      calcgdSizeActive();
      calclaSize();
      calcrFactorSize();

      updateWRef(WParent[0](Index(0,getuSize()-1),Index(0,getlaSize()-1)));
      updateVRef(VParent[0](Index(0,getuSize()-1),Index(0,getlaSize()-1)));
      updatelaRef(laParent(0,laSize-1));
      updategdRef(gdParent(0,gdSize-1));
      if(impactSolver==RootFinding) updateresRef(resParent(0,laSize-1));
      updaterFactorRef(rFactorParent(0,rFactorSize-1));

    }
    updategd(t);

    updateT(t); 
    updateJacobians(t);
    updateh(t); 
    updateM(t); 
    facLLM(); 
    updateW(t); 
    updateV(t); 
    updateG(t); 
  }

  void DynamicSystemSolver::zdot(const Vec &zParent, Vec &zdParent, double t) {
    if(qd()!=zdParent()) {
      updatezdRef(zdParent);
    }
    zdot(zParent,t);
  }

 void DynamicSystemSolver::getLinkStatus(Vector<int> &LinkStatusExt, double t) {
    if(LinkStatusExt.size()<LinkStatusSize) 
      LinkStatusExt.resize(LinkStatusSize, INIT, 0);
    if(LinkStatus()!=LinkStatusExt())
      updateLinkStatusRef(LinkStatusExt);
    updateLinkStatus(t);
  }

  void DynamicSystemSolver::projectGeneralizedPositions(double t) {
    if(laSize) {
      calcgSizeActive();

      updategRef(gParent(0,gSize-1));
      updateg(t);
      Vec nu(getuSize());
      calclaSizeForActiveg();
      updateWRef(WParent[0](Index(0,getuSize()-1),Index(0,getlaSize()-1)));
      updateW(t);
      Vec corr;
      corr = g;
      corr.init(0*tolProj);
      SqrMat Gv= SqrMat(W[0].T()*slvLLFac(LLM[0],W[0])); 
      // TODO: Wv*T check
      while(nrmInf(g-corr) >= tolProj) {
        Vec mu = slvLS(Gv, -g+W[0].T()*nu+0.5*corr);
        Vec dnu = slvLLFac(LLM[0],W[0]*mu)-nu;
        nu += dnu;
        q += T*dnu;
        updateStateDependentVariables(t);
        updateg(t);
      }
      calclaSize();
      updateWRef(WParent[0](Index(0,getuSize()-1),Index(0,getlaSize()-1)));
      calcgSize();
      updategRef(gParent(0,gSize-1));
    }
  }

  void DynamicSystemSolver::projectGeneralizedVelocities(double t) {
    if(laSize) {
      calcgdSizeActive();
      updategdRef(gdParent(0,gdSize-1));
      updategd(t);

      SqrMat Gv= SqrMat(W[0].T()*slvLLFac(LLM[0],W[0])); 
      Vec mu = slvLS(Gv,-gd);
      u += slvLLFac(LLM[0],W[0]*mu);
      calcgdSize();
      updategdRef(gdParent(0,gdSize-1));
    }
  }

  void DynamicSystemSolver::savela(double dt) {
    for(vector<Link*>::iterator i = linkSetValuedActive.begin(); i != linkSetValuedActive.end(); ++i) 
      (**i).savela(dt);
  }

  void DynamicSystemSolver::initla(double dt) {
    for(vector<Link*>::iterator i = linkSetValuedActive.begin(); i != linkSetValuedActive.end(); ++i) 
      (**i).initla(dt);
  }

  double DynamicSystemSolver::computePotentialEnergy() {
    double Vpot = 0.0;

    vector<Object*>::iterator i;
    for(i = object.begin(); i != object.end(); ++i) Vpot += (**i).computePotentialEnergy();

    vector<Link*>::iterator ic;
    for(ic = link.begin(); ic != link.end(); ++ic) Vpot += (**ic).computePotentialEnergy();
    return Vpot;
  }

  void DynamicSystemSolver::addElement(Element *element_) {
    Object* object_=dynamic_cast<Object*>(element_);
    Link* link_=dynamic_cast<Link*>(element_);
    ExtraDynamic* ed_=dynamic_cast<ExtraDynamic*>(element_);
    if(object_) addObject(object_);
    else if(link_) addLink(link_);
    else if(ed_) addExtraDynamic(ed_);
    else{ throw MBSimError("ERROR (DynamicSystemSolver: addElement()): No such type of Element to add!");}
  }

  Element* DynamicSystemSolver::getElement(const string &name) {
    //   unsigned int i1;
    //   for(i1=0; i1<object.size(); i1++) {
    //     if(object[i1]->getName() == name) return (Element*)object[i1];
    //   }
    //   for(i1=0; i1<object.size(); i1++) {
    //     if(object[i1]->getPath() == name) return (Element*)object[i1];
    //   }
    //   unsigned int i2;
    //   for(i2=0; i2<link.size(); i2++) {
    //     if(link[i2]->getName() == name) return (Element*)link[i2];
    //   }
    //   for(i2=0; i2<link.size(); i2++) {
    //     if(link[i2]->getPath() == name) return (Element*)link[i2];
    //   }
    //   unsigned int i3;
    //   for(i3=0; i3<extraDynamic.size(); i3++) {
    //     if(extraDynamic[i3]->getName() == name) return (Element*)extraDynamic[i3];
    //   }
    //   for(i3=0; i3<extraDynamic.size(); i3++) {
    //     if(extraDynamic[i3]->getPath() == name) return (Element*)extraDynamic[i3];
    //   }
    //   if(!(i1<object.size())||!(i2<link.size())||!(i3<extraDynamic.size())) cout << "Error: The DynamicSystemSolver " << this->name <<" comprises no element " << name << "!" << endl; 
    //   assert(i1<object.size()||i2<link.size()||!(i3<extraDynamic.size()));
    return NULL;
  }

  string DynamicSystemSolver::getSolverInfo() {
    stringstream info;

    if(impactSolver == GaussSeidel) info << "GaussSeidel";
    else if(impactSolver == LinearEquations) info << "LinearEquations";
    else if(impactSolver == FixedPointSingle) info << "FixedPointSingle";
    else if(impactSolver == FixedPointTotal) info << "FixedPointTotal";
    else if(impactSolver == RootFinding) info << "RootFinding";

    // Gauss-Seidel & solveLL do not depend on the following ...
    if(impactSolver!=GaussSeidel && impactSolver!=LinearEquations) {
      info << "(";

      // r-Factor strategy
      if(strategy==global) info << "global";
      else if(strategy==local) info << "local";

      // linear algebra for RootFinding only
      if(impactSolver == RootFinding) {
        info << ",";
        if(linAlg==LUDecomposition) info << "LU";
        else if(linAlg==LevenbergMarquardt) info << "LM";
        else if(linAlg==PseudoInverse) info << "PI";
      }
      info << ")";
    }
    return info.str();
  }

  void DynamicSystemSolver::dropContactMatrices() {
    cout << "dropping contact matrices to file <dump_matrices.asc>" << endl;
    ofstream contactDrop("dump_matrices.asc");   

    contactDrop << "constraint functions g" << endl << g.T() << endl << endl;
    contactDrop << endl;
    contactDrop << "mass matrix M" << endl << M << endl << endl;
    contactDrop << "generalized force directions W" << endl << W << endl << endl;
    contactDrop << "Delassus matrix G" << endl << G << endl << endl;
    contactDrop << endl;
    contactDrop << "constraint velocities gp" << endl << gd.T() << endl << endl;
    contactDrop << "non-holonomic part in gp; b" << endl << b.T() << endl << endl;
    contactDrop << "Lagrange multipliers la" << endl << la.T() << endl << endl;
    contactDrop.close();
  }

  void DynamicSystemSolver::sigInterruptHandler(int) {
    cout<<"MBSim: Received user interrupt or terminate signal!"<<endl;
    exitRequest=true;
  }

  void DynamicSystemSolver::sigAbortHandler(int) {
    cout<<"MBSim: Received abort signal! Flushing HDF5 files and abort!"<<endl;
    H5::FileSerie::flushAllFiles();
  }

  void DynamicSystemSolver::writez(string fileName, bool formatH5) {
    if (formatH5) {
      H5::H5File file(fileName, H5F_ACC_TRUNC);

      H5::SimpleDataSet<vector<double> > qToWrite;
      qToWrite.create(file,"q0");
      qToWrite.write(q);

      H5::SimpleDataSet<vector<double> > uToWrite;
      uToWrite.create(file,"u0");
      uToWrite.write(u);

      H5::SimpleDataSet<vector<double> > xToWrite;
      xToWrite.create(file,"x0");
      xToWrite.write(x);

      file.close();
    }
    else {
      ofstream file(fileName.c_str());
      file.setf(ios::scientific);
      file.precision(15);
      for (int i=0; i<q.size(); i++)
        file << q(i) << endl;
      for (int i=0; i<u.size(); i++)
        file << u(i) << endl;
      for (int i=0; i<x.size(); i++)
        file << x(i) << endl;
      file.close();
    }
  }

  void DynamicSystemSolver::readz0(string fileName) {
    H5::H5File file(fileName, H5F_ACC_RDONLY);

    H5::SimpleDataSet<vector<double> > qToRead;
    qToRead.open(file,"q0");
    q0 = qToRead.read();

    H5::SimpleDataSet<vector<double> > uToRead;
    uToRead.open(file,"u0");
    u0 = uToRead.read();

    H5::SimpleDataSet<vector<double> > xToRead;
    xToRead.open(file,"x0");
    x0 = xToRead.read();

    file.close();

    READZ0 = true;
  }

  void DynamicSystemSolver::updatezRef(const Vec &zParent) {

    q >> ( zParent(0,qSize-1) );
    u >> ( zParent(qSize,qSize+uSize[0]-1) );
    x >> ( zParent(qSize+uSize[0],qSize+uSize[0]+xSize-1) );

    updateqRef(q);
    updateuRef(u);
    updatexRef(x);
  }

  void DynamicSystemSolver::updatezdRef(const Vec &zdParent) {

    qd >> ( zdParent(0,qSize-1) );
    ud[0] >> ( zdParent(qSize,qSize+uSize[0]-1) );
    xd >> ( zdParent(qSize+uSize[0],qSize+uSize[0]+xSize-1) );

    updateqdRef(qd);
    updateudRef(ud[0]);
    updatexdRef(xd);

    updateudallRef(ud[0]);
  }

  void DynamicSystemSolver::updaterFactors() {
    if(strategy == global) {
      //     double rFac;
      //     if(G.size() == 1) rFac = 1./G(0,0);
      //     else {
      //       Vec eta = eigvalSel(G,1,G.size());
      //       double etaMax = eta(G.size()-1);
      //       double etaMin = eta(0);
      //       int i=1;
      //       while(abs(etaMin) < 1e-8 && i<G.size()) etaMin = eta(i++);
      //       rFac = 2./(etaMax + etaMin);
      //     }
      //     rFactor.init(rFac);

      throw MBSimError("ERROR (DynamicSystemSolver::updaterFactors()): Global r-Factor strategy not currently not available.");
    }
    else if(strategy == local) Group::updaterFactors();
    else throw MBSimError("ERROR (DynamicSystemSolver::updaterFactors()): Unknown strategy.");
  }

  void DynamicSystemSolver::computeConstraintForces(double t) {
    la = slvLS(G, -(W[0].T()*slvLLFac(LLM[0],h[0]) + wb)); // slvLS because of undeterminded system of equations
  } 

  void DynamicSystemSolver::constructor() {
    integratorExitRequest=false;
    setPlotFeatureRecursive(plotRecursive, enabled);
    setPlotFeature(separateFilePerGroup, enabled);
    setPlotFeatureForChildren(separateFilePerGroup, disabled);
    setPlotFeatureRecursive(openMBV, enabled);
  }

  void DynamicSystemSolver::initializeUsingXML(TiXmlElement *element) {
    Group::initializeUsingXML(element);
    TiXmlElement *e;
    // search first Environment element
    e=element->FirstChildElement(MBSIMNS"environments")->FirstChildElement();

    Environment *env;
    while((env=ObjectFactory::getInstance()->getEnvironment(e))) {
      env->initializeUsingXML(e);
      e=e->NextSiblingElement();
    }

    e=element->FirstChildElement(MBSIMNS"solverParameters");
    if (e) {
      TiXmlElement * ee;
      ee=e->FirstChildElement(MBSIMNS"constraintSolver");
      if (ee) {
        if (ee->FirstChildElement(MBSIMNS"FixedPointTotal"))
          setConstraintSolver(FixedPointTotal);
        else if (ee->FirstChildElement(MBSIMNS"FixedPointSingle"))
          setConstraintSolver(FixedPointSingle);
        else if (ee->FirstChildElement(MBSIMNS"GaussSeidel"))
          setConstraintSolver(GaussSeidel);
        else if (ee->FirstChildElement(MBSIMNS"LinearEquations"))
          setConstraintSolver(LinearEquations);
        else if (ee->FirstChildElement(MBSIMNS"RootFinding"))
          setConstraintSolver(RootFinding);
      }
      ee=e->FirstChildElement(MBSIMNS"impactSolver");
      if (ee) {
        if (ee->FirstChildElement(MBSIMNS"FixedPointTotal"))
          setImpactSolver(FixedPointTotal);
        else if (ee->FirstChildElement(MBSIMNS"FixedPointSingle"))
          setImpactSolver(FixedPointSingle);
        else if (ee->FirstChildElement(MBSIMNS"GaussSeidel"))
          setImpactSolver(GaussSeidel);
        else if (ee->FirstChildElement(MBSIMNS"LinearEquations"))
          setImpactSolver(LinearEquations);
        else if (ee->FirstChildElement(MBSIMNS"RootFinding"))
          setImpactSolver(RootFinding);
      }
      ee=e->FirstChildElement(MBSIMNS"numberOfMaximalIterations");
      if (ee)
        setMaxIter(atoi(ee->GetText()));
      ee=e->FirstChildElement(MBSIMNS"tolerances");
      if (ee) {
        TiXmlElement * eee;
        eee=ee->FirstChildElement(MBSIMNS"projection");
        if (eee)
          setProjectionTolerance(getDouble(eee));
        eee=ee->FirstChildElement(MBSIMNS"gd");
        if (eee)
          setgdTol(getDouble(eee));
        eee=ee->FirstChildElement(MBSIMNS"gdd");
        if (eee)
          setgddTol(getDouble(eee));
        eee=ee->FirstChildElement(MBSIMNS"la");
        if (eee)
          setlaTol(getDouble(eee));
        eee=ee->FirstChildElement(MBSIMNS"La");
        if (eee)
          setLaTol(getDouble(eee));
      }
    }
  }

  void DynamicSystemSolver::addToGraph(Graph* graph, SqrMat &A, int i, vector<Object*>& objList) {
    graph->addObject(objList[i]->computeLevel(),objList[i]);
    A(i,i) = -1;

    for(int j=0; j<A.cols(); j++)
      if(A(i,j) > 0 && fabs(A(j,j)+1)>epsroot()) // child node of object i
        addToGraph(graph, A, j, objList);
  }

  void DynamicSystemSolver::shift(Vec &zParent, const Vector<int> &jsv_, double t) {
    if(q()!=zParent()) {
      updatezRef(zParent);
    }
    jsv = jsv_;

   // cout << endl << "shift at" <<  endl;
   // cout << t << endl;
   // cout << sv << endl;
   // cout << jsv << endl;
   // cout << "laSize = " << laSize << endl;
   // cout << "gdSize = " << gdSize << endl;
   // cout << "g = " << endl << g << endl;
   // cout << "gd = " << endl << gd << endl;
    updateCondition(); // check for impact

    if(impact) { // impact

      updateStateDependentVariables(t);
      updateg(t);
   //   cout << "g = " << endl << g << endl;
      checkActiveg();
      checkActiveLinks(); // list with active links (g<=0)
      checkAllgd(); // look at all closed contacts
      calcgdSizeActive(); // Prüfen
      updategdRef(gdParent(0,gdSize-1));
      calclaSize();
 //     cout << "laSize = " << laSize << endl;
 //     cout << "gdSize = " << gdSize << endl;
      calcrFactorSize();
      updateWRef(WParent[0](Index(0,getuSize()-1),Index(0,getlaSize()-1)));
      updateVRef(VParent[0](Index(0,getuSize()-1),Index(0,getlaSize()-1)));
      updatelaRef(laParent(0,laSize-1));
      updaterFactorRef(rFactorParent(0,rFactorSize-1));

      updateStateDependentVariables(t); // TODO necessary?
      updateg(t); // TODO necessary?
      updategd(t); // important because of updategdRef
      updateJacobians(t);
      updateW(t); // important because of updateWRef
      updateV(t); 
      updateG(t); 
      // projectGeneralizedPositions(t);
      b.resize() = gd; // b = gd + trans(W)*slvLLFac(LLM,h)*dt with dt=0
      int iter;
      iter = solveImpacts();
      //cout << "vorher: u = " << endl << u << endl;
      u += deltau(zParent,t,0);
      //cout << "nacher: u = " << endl << u << endl;
      calcgdSize();
      //cout << "gdSize = " << gdSize << endl;
      updategdRef(gdParent(0,gdSize-1));
    }
    updateStateDependentVariables(t); // TODO necessary?
    if(alwaysConsiderContact) {
      updateg(t); // TODO necessary?
      updategd(t);
      checkState();
    } else {
      updateg(t); // TODO necessary?
      checkActiveg();
      checkActiveLinks(); // TODO
      updategd(t);
      checkActivegd();
      checkState();
    }
    impact = false;

  }

  void DynamicSystemSolver::getsv(const fmatvec::Vec& zParent, fmatvec::Vec& svExt, double t) {
    if(sv()!=svExt()) {
      updatesvRef(svExt);
    }

    if(q()!=zParent())
      updatezRef(zParent);

    if(qd()!=zdParent()) 
      updatezdRef(zdParent);

    updateStateDependentVariables(t);
    updateg(t);
    if(alwaysConsiderContact) {
      checkActiveg();
      checkActiveLinks();
      updategd(t);
      checkActivegd();
    } else {
      checkActiveLinks();
      updategd(t);
    }
    updateStopVector(t);
    sv(sv.size()-1) = 1;
  }

  fmatvec::Vec DynamicSystemSolver::zdot(const fmatvec::Vec &zParent, double t) {
    if(q()!=zParent()) {
      updatezRef(zParent);
    }
    updateStateDependentVariables(t);
    updateg(t);
    if(alwaysConsiderContact) {
      checkActiveg();
      checkActiveLinks();
      updategd(t);
      checkActivegd();
    } else {
      checkActiveLinks(); // TODO: nicht nötig
      updategd(t);
    }
    updateT(t); 
    updateJacobians(t);
    updateh(t); 
    updateM(t); 
    facLLM(); 
    calclaSize();
    calcrFactorSize();
    updateWRef(WParent[0](Index(0,getuSize()-1),Index(0,getlaSize()-1)));
    updateVRef(VParent[0](Index(0,getuSize()-1),Index(0,getlaSize()-1)));
    updatelaRef(laParent(0,laSize-1));
    updatewbRef(wbParent(0,laSize-1));
    updaterFactorRef(rFactorParent(0,rFactorSize-1));
    if(laSize) {
      updateW(t); 
      updateV(t); 
      updateG(t); 
      updatewb(t); 
      b.resize() = W[0].T()*slvLLFac(LLM[0],h[0]) + wb;
      int iter;
      iter = solveConstraints();
    }
    updater(t); 
    updatezd(t);
    return zdParent;
  }

  void DynamicSystemSolver::plot(const fmatvec::Vec& zParent, double t, double dt) {
    if(q()!=zParent()) {
      updatezRef(zParent);
    }

    if(qd()!=zdParent()) 
      updatezdRef(zdParent);
    updateStateDependentVariables(t);
    updateg(t);
    if(alwaysConsiderContact) {
      checkActiveg();
      checkActiveLinks();
      updategd(t);
      checkActivegd();
    } else {
      checkActiveLinks();
      updategd(t);
    }
    updateT(t); 
    updateJacobians(t,0);
    updateJacobians(t,1);
    updateh(t,1);
    buf.resize(3,20);
    updateh0Fromh1(t);
    updateM(t,0); 
    facLLM(0); 
    calclaSize();
    calcrFactorSize();
      updateWRef(WParent[1](Index(0,getuSize(1)-1),Index(0,getlaSize()-1)),1);
      updateVRef(VParent[1](Index(0,getuSize(1)-1),Index(0,getlaSize()-1)),1);
      updatelaRef(laParent(0,laSize-1));
      updatewbRef(wbParent(0,laSize-1));
      updaterFactorRef(rFactorParent(0,rFactorSize-1));
      updateWRef(WParent[0](Index(0,getuSize(0)-1),Index(0,getlaSize()-1)),0);
      updateVRef(VParent[0](Index(0,getuSize(0)-1),Index(0,getlaSize()-1)),0);
    if(laSize) {
      updateW(t,1); 
      updateV(t,1); 
      updateWnVRefObjects();
      updateW0FromW1(t);
      updateV0FromV1(t);
      updateG(t); 
      updatewb(t); 
      b.resize() = W[0].T()*slvLLFac(LLM[0],h[0]) + wb;
      int iter;
      iter = solveConstraints();
    }

    updater(t,0); 
    updatezd(t);
    //updateStateDerivativeDependentVariables(t);

    updategInverseKinetics(t); // TODO not optimal, but necessary because of update of force direction
    updategdInverseKinetics(t); // TODO not optimal, but necessary because of update of force direction
    updateJacobiansInverseKinetics(t,1);
    updater(t,1);
    updatehInverseKinetics(t,1); // Accelerations of objects
    updateWInverseKinetics(t,1); 
    ///updateWInverseKinetics(t,0); 
    updatebInverseKinetics(t); 

    //if(WInverseKinetics[1].cols() >0)
  //    cout << WInverseKinetics[0] << endl;
    //  laInverseKinetics = slvLL(JTJ(WInverseKinetics[1]),-WInverseKinetics[1].T()*(h[1]+r[1]));
      int n = WInverseKinetics[1].cols();
      int m1 = WInverseKinetics[1].rows();
      int m2 = bInverseKinetics.rows();
      Mat A(m1+m2,n);
      Vec b(m1+m2);
    A(Index(0,m1-1),Index(0,n-1)) = WInverseKinetics[1];
    A(Index(m1,m1+m2-1),Index(0,n-1)) = bInverseKinetics;
      b(0,m1-1) = -h[1]-r[1];
      Vec x =  slvLL(JTJ(A),A.T()*b);
      laInverseKinetics = x(0,n-1);

    DynamicSystemSolver::plot(t,dt);

    if(++flushCount > flushEvery) {
      flushCount = 0;
      H5::FileSerie::flushAllFiles();
    }

  }

  fmatvec::Vec MySolver3::zdot(const fmatvec::Vec &zParent, double t) {
    if(q()!=zParent()) {
      updatezRef(zParent);
    }
    updateStateDependentVariables(t);
    updateg(t);
    if(alwaysConsiderContact) {
      checkActiveg();
      checkActiveLinks();
      updategd(t);
      checkActivegd();
    } else {
      checkActiveLinks(); // TODO: nicht nötig
      updategd(t);
    }
    updateT(t); 
    updateJacobians(t,1);
    updateh(t,1); 
    updateM(t,1); 
    facLLM(1); 
    calclaSize();
    calcrFactorSize();
    //updateWRef(WParent[0](Index(0,getuSize()-1),Index(0,getlaSize()-1)));
    //updateVRef(VParent[0](Index(0,getuSize()-1),Index(0,getlaSize()-1)));
    //updatelaRef(laParent(0,laSize-1));
    //updatewbRef(wbParent(0,laSize-1));
    //updaterFactorRef(rFactorParent(0,rFactorSize-1));
    if(laSize) {
      updateW(t,1); 
      updateV(t,1); 
      updateG(t,1); 
      updatewb(t,1); 
      b.resize() = W[1].T()*slvLLFac(LLM[1],h[1]) + wb;
      int iter;
      iter = solveConstraints();
      cout << "la= " << la << endl;
    }
    updateJacobians(t,0);
    //updateh(t);
    updateh0Fromh1(t);
    updateM(t);
    facLLM();
    updateW(t); 
    updateV(t); 
    updateG(t); 
    updatewb(t); 
    b.resize() = W[0].T()*slvLLFac(LLM[0],h[0]) + wb;
    int iter;
    iter = solveConstraints();
    cout << "la= " << la << endl;
    updater(t); 
    updatezd(t);
    cout << t << endl;
    cout << qd << endl;
    cout << ud[0] << endl;
    cout << ud[1] << endl;
    //updatezd(t);
    return zdParent;
  }

  void MySolver3::plot(const fmatvec::Vec& zParent, double t, double dt) {
    if(q()!=zParent()) {
      updatezRef(zParent);
    }

    if(qd()!=zdParent()) 
      updatezdRef(zdParent);
    updateStateDependentVariables(t);
    updateg(t);
    if(alwaysConsiderContact) {
      checkActiveg();
      checkActiveLinks();
      updategd(t);
      checkActivegd();
    } else {
      checkActiveLinks(); // TODO: nicht nötig
      updategd(t);
    }
    updateT(t); 
    updateJacobians(t,1);
    updateh(t,1); 
    updateM(t,1); 
    facLLM(1); 
    calclaSize();
    calcrFactorSize();
    //updateWRef(WParent[0](Index(0,getuSize()-1),Index(0,getlaSize()-1)));
    //updateVRef(VParent[0](Index(0,getuSize()-1),Index(0,getlaSize()-1)));
    //updatelaRef(laParent(0,laSize-1));
    //updatewbRef(wbParent(0,laSize-1));
    //updaterFactorRef(rFactorParent(0,rFactorSize-1));
    if(laSize) {
      updateW(t,1); 
      updateV(t,1); 
      updateG(t,1); 
      updatewb(t,1); 
      b.resize() = W[1].T()*slvLLFac(LLM[1],h[1]) + wb;
      int iter;
      iter = solveConstraints();
      cout << "la= " << la << endl;
    }
    //  updateJacobians(t,0);
    //  updateh(t);
    //  updateM(t);
    //  facLLM();
    //  updateW(t); 
    //  updateV(t); 
    //  updateG(t); 
    //  updatewb(t); 
    //  b.resize() = W[0].T()*slvLLFac(LLM[0],h[0]) + wb;
    //  int iter;
    //  iter = solveConstraints();
    //    cout << "la= " << la << endl;
    //  updater(t); 
    DynamicSystemSolver::plot(t,dt);

    if(++flushCount > flushEvery) {
      flushCount = 0;
      H5::FileSerie::flushAllFiles();
    }

  }

}

