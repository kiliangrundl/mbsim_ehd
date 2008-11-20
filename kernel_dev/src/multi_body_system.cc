/* Copyright (C) 2004-2006  Martin Förg, Roland Zander
 
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
 *   rzander@users.berlios.de
 *
 */
#include<config.h>
#include "multi_body_system.h"
#include "coordinate_system.h"
#include "contour.h"
#include "link.h"
#include "extra_dynamic_interface.h"
#include "integrator.h"
#include "body_flexible.h"
#include "eps.h"
#include "dirent.h"
#ifndef MINGW
#  include<sys/stat.h>
#else
#  include<io.h>
#  define mkdir(a,b) mkdir(a)
#endif

namespace MBSim {

  MultiBodySystem::MultiBodySystem() : Group("Default"), grav(3), maxIter(10000), highIter(1000), maxDampingSteps(3), lmParm(0.001), warnLevel(0), contactSolver(FixedPointSingle), impactSolver(FixedPointSingle), strategy(local), linAlg(LUDecomposition), stopIfNoConvergence(false), dropContactInfo(false), useOldla(true), numJac(false), checkGSize(true), limitGSize(500), directoryName("Default"), preIntegrator(NULL), peds(false), impact(false), sticking(false) { //, activeConstraintsChanged(true)

  } 

  MultiBodySystem::MultiBodySystem(const string &projectName) : Group(projectName), grav(3), maxIter(10000), highIter(1000), maxDampingSteps(3), lmParm(0.001), warnLevel(0), contactSolver(FixedPointSingle), impactSolver(FixedPointSingle), strategy(local), linAlg(LUDecomposition), stopIfNoConvergence(false), dropContactInfo(false), useOldla(true), numJac(false), checkGSize(true), limitGSize(500), directoryName("Default") , preIntegrator(NULL), peds(false), impact(false), sticking(false)  { //, activeConstraintsChanged(true)

  }

  MultiBodySystem::~MultiBodySystem() {
    if (preIntegrator) delete preIntegrator;
  } 

  void MultiBodySystem::init() {
    setUpLinks();

    setMultiBodySystem(this);
    setFullName(name);

    setDirectory(); // output directory

    calcqSize();
    calcuSize();
    calcxSize();
    sethSize(uSize);

    calclaSize();
    calcgSize();
    calcgdSize();
    calcrFactorSize();
    calcsvSize();

    cout << "qSize = " << qSize <<endl;
    cout << "uSize = " << uSize <<endl;
    cout << "xSize = " << xSize <<endl;
    cout << "gSize = " << gSize <<endl;
    cout << "qdSize = " << gdSize <<endl;
    cout << "laSize = " << laSize <<endl;
    cout << "svSize = " << svSize <<endl;

    setlaIndMBS(laInd);

    // TODO Speicherproblem bei vielen moeglichen Kontakten
    if(laSize>8000)
      laSize=8000;
    MParent.resize(getuSize());
    TParent.resize(getqSize(),getuSize());
    LLMParent.resize(getuSize());
    WParent.resize(getuSize(),getlaSize());
    VParent.resize(getuSize(),getlaSize());
    wbParent.resize(getlaSize());
    laParent.resize(getlaSize());
    rFactorParent.resize(getlaSize());
    sParent.resize(getlaSize());
    resParent.resize(getlaSize());
    gParent.resize(getgSize());
    gdParent.resize(getgdSize());
    zdParent.resize(getzSize());
    hParent.resize(getuSize());
    rParent.resize(getuSize());
    fParent.resize(getxSize());
    svParent.resize(getsvSize());
    jsvParent.resize(getsvSize());

    updateMRef(MParent);
    updateTRef(TParent);
    updateLLMRef(LLMParent);

    Group::init();

    updatesvRef(svParent);
    updatejsvRef(jsvParent);
    updatezdRef(zdParent);
    updatelaRef(laParent);
    updategRef(gParent);
    updategdRef(gdParent);
    updatehRef(hParent);
    updaterRef(rParent);
    updateWRef(WParent);
    updateVRef(VParent);
    updatewbRef(wbParent);
    updategRef(gParent);
    updategdRef(gdParent);
    updatelaRef(laParent);
    updaterFactorRef(rFactorParent);


    // contact solver specific settings
    cout << "  use contact solver \'" << getSolverInfo() << "\' for contact situations" << endl;
    if(contactSolver == GaussSeidel) solveConstraints_ = &MultiBodySystem::solveConstraintsGaussSeidel; 
    else if(contactSolver == LinearEquations) {
      solveConstraints_ = &MultiBodySystem::solveConstraintsLinearEquations;
      cout << "WARNING: solveLL is only valid for bilateral constrained systems!" << endl;
    }
    else if(contactSolver == FixedPointSingle) solveConstraints_ = &MultiBodySystem::solveConstraintsFixpointSingle;
    else if(contactSolver == RootFinding)solveConstraints_ = &MultiBodySystem::solveConstraintsRootFinding;
    else {
      cout << "Error: unknown contact solver" << endl;
      throw 5;
    }

    // impact solver specific settings
    cout << "  use impact solver \'" << getSolverInfo() << "\' for impact situations" << endl;
    if(impactSolver == GaussSeidel) solveImpacts_ = &MultiBodySystem::solveImpactsGaussSeidel; 
    else if(impactSolver == LinearEquations) {
      solveImpacts_ = &MultiBodySystem::solveImpactsLinearEquations;
      cout << "WARNING: solveLL is only valid for bilateral constrained systems!" << endl;
    }
    else if(impactSolver == FixedPointSingle) solveImpacts_ = &MultiBodySystem::solveImpactsFixpointSingle;
    else if(impactSolver == RootFinding)solveImpacts_ = &MultiBodySystem::solveImpactsRootFinding;
    else {
      cout << "Error: unknown impact solver" << endl;
      throw 5;
    }

    // if(plotting) {
    cout << "  initialising plot-files ..." << endl;
    initPlotFiles();
    // }

    cout << "...... done initialising." << endl << endl;

  }

  void MultiBodySystem::setDirectory() {
    // SETDIRECTORY creates directories for outputs

    int i;
    string projectDirectory;

    if(false) { // TODO: introduce flag "overwriteDirectory"
      for(i=0; i<=99; i++) {
	stringstream number;
	number << "." << setw(2) << setfill('0') << i;
	projectDirectory = directoryName + number.str();
	int ret = mkdir(projectDirectory.c_str(),0777);
	if(ret == 0) break;
      }
      cout << "  make directory \'" << projectDirectory << "\' for output processing" << endl;
    }
    else { // always the same directory
      projectDirectory = string(directoryName);

      int ret = mkdir(projectDirectory.c_str(),0777);
      if(ret == 0) {
	cout << "  make directory \'" << projectDirectory << "\' for output processing" << endl;
      }
      else {
	cout << "  use existing directory \'" << projectDirectory << "\' for output processing" << endl;
      }
    }

    dirName = projectDirectory+"/";

    if(preIntegrator) {
      string preDir="PREINTEG";
      int ret=mkdir(preDir.c_str(),0777);
      if(ret==0) {
	cout << "Make directory " << preDir << " for Preintegration results." << endl;
      }
      else {
	cout << "Use existing directory " << preDir << " for Preintegration results." << endl;
      }
    }

    return;
  }

  void MultiBodySystem::computeInitialCondition() {
    updateKinematics(0);
    updateg(0);
    checkActiveg();
    updategd(0);
    checkActivegd();
    checkActiveLinks();
    //cout <<endl<< "activeConstraintsChanged at t = " << 0 << endl<<endl;
    calclaSize();
    calcrFactorSize();
    setlaIndMBS(laInd);
    updateWRef(WParent(Index(0,getuSize()-1),Index(0,getlaSize()-1)));
    updateVRef(VParent(Index(0,getuSize()-1),Index(0,getlaSize()-1)));
    updatelaRef(laParent(0,laSize-1));
    updatewbRef(wbParent(0,laSize-1));
    updaterFactorRef(rFactorParent(0,rFactorSize-1));
    //cout << laSize<<endl;
  }

  Vec MultiBodySystem::zdot(const Vec &zParent, double t) {
    if(q()!=zParent()) {
      updatezRef(zParent);
    }
    updateKinematics(t);
    updateg(t);
    updategd(t);
    updateT(t); 
    updateh(t); 
    updateM(t); 
    facLLM(); 
    if(laSize) {
      updateW(t); 
      updateV(t); 
      updateG(t); 
      updatewb(t); 
      computeConstraintForces(t); // Alt
    }
    updater(t); 
    updatezd(t);
    return zdParent;
  }

  void MultiBodySystem::updatezRef(const Vec &zParent) {

    q >> ( zParent(0,qSize-1) );
    u >> ( zParent(qSize,qSize+uSize-1) );
    x >> ( zParent(qSize+uSize,qSize+uSize+xSize-1) );

    for(vector<Subsystem*>::iterator i = subsystem.begin(); i != subsystem.end(); ++i) {
      (**i).updateqRef(q);
      (**i).updateuRef(u);
      (**i).updatexRef(x);
    }

    for(vector<Object*>::iterator i = object.begin(); i != object.end(); ++i) {
      (**i).updateqRef(q);
      (**i).updateuRef(u);
    }

    for(vector<Link*>::iterator i = link.begin(); i != link.end(); ++i) 
      (**i).updatexRef(x);

    for(vector<ExtraDynamicInterface*>::iterator i = EDI.begin(); i != EDI.end(); ++i) 
      (**i).updatexRef(x);
  }

  void MultiBodySystem::updatezdRef(const Vec &zdParent) {

    qd >> ( zdParent(0,qSize-1) );
    ud >> ( zdParent(qSize,qSize+uSize-1) );
    xd >> ( zdParent(qSize+uSize,qSize+uSize+xSize-1) );

    for(vector<Subsystem*>::iterator i = subsystem.begin(); i != subsystem.end(); ++i) {
      (**i).updateqdRef(qd);
      (**i).updateudRef(ud);
      (**i).updatexdRef(xd);
    }

    for(vector<Object*>::iterator i = object.begin(); i != object.end(); ++i) {
      (**i).updateqdRef(qd);
      (**i).updateudRef(ud);
    }

    for(vector<Link*>::iterator i = link.begin(); i != link.end(); ++i) 
      (**i).updatexdRef(xd);

    for(vector<ExtraDynamicInterface*>::iterator i = EDI.begin(); i != EDI.end(); ++i) 
      (**i).updatexdRef(xd);
  }

  void MultiBodySystem::zdot(const Vec &zParent, Vec &zdParent, double t) {
    if(qd()!=zdParent()) {
      updatezdRef(zdParent);
    }
    zdot(zParent,t);
  }

  void MultiBodySystem::plot(const Vec& zParent, double t, double dt) {
    if(q()!=zParent()) {
      updatezRef(zParent);
    }

    if(qd()!=zdParent()) 
      updatezdRef(zdParent);

    updateKinematics(t);
    updateg(t);
    updategd(t);
    // TODO nötig für ODE-Integration und hohem plotLevel
    // for(vector<Link*>::iterator iL = linkSetValued.begin(); iL != linkSetValued.end(); ++iL) 
    //  (*iL)->updateStage2(t);
    updateh(t); 
    updateM(t); 
    //updateG(t); 
    //computeConstraintForces(t); 
    //updater(t); 
    //updatezd(t); 


    plot(t,dt);
  }

  void MultiBodySystem::initz(Vec& z) {
    // INITZ initialises the state of object and EDIs for whole multibody system
    updatezRef(z);
    Group::initz();
  }

  double MultiBodySystem::computePotentialEnergy() {
    // COMPUTEPOTENTIALENERGY computes potential energy
    double Vpot = 0.0;

    vector<Object*>::iterator i;
    for(i = object.begin(); i != object.end(); ++i) Vpot += (**i).computePotentialEnergy();

    vector<Link*>::iterator ic;
    for(ic = link.begin(); ic != link.end(); ++ic) Vpot += (**ic).computePotentialEnergy();
    return Vpot;
  }

  void MultiBodySystem::getsv(const Vec& zParent, Vec& svExt, double t) { 
    if(sv()!=svExt()) {
      updatesvRef(svExt);
      //sv.init(1);
    }

    if(q()!=zParent())
      updatezRef(zParent);

    if(qd()!=zdParent()) 
      updatezdRef(zdParent);

    updateKinematics(t);
    updateg(t);
    updategd(t);
    updateT(t); 
    updateh(t); 
    updateM(t); 
    facLLM(); 
    if(laSize) {
      updateW(t); 
      updateV(t); 
      updateG(t); 
      updatewb(t); 
      computeConstraintForces(t); // Alt
    }
    updateStopVector(t);
  }

  void MultiBodySystem::setAccelerationOfGravity(const Vec& g) {
    // SETGRAV sets gravitation
    grav = g;
  }

  void MultiBodySystem::update(const Vec &zParent, double t) {
    // UPDATE updates the position depending structures for multibody system

    if(q()!=zParent()) updatezRef(zParent);

    updateKinematics(t);
    updateg(t);
    checkActiveg();
    checkActiveLinks();
    if(gActiveChanged()) {
      //cout << "activeConstraintsChanged at t = " << t << endl;

      checkAllgd(); // Prüfen ob nötig
      calcgdSize();
      calclaSize();
      calcrFactorSize();

      setlaIndMBS(laInd);

      //cout << laSize << endl;
      updateWRef(WParent(Index(0,getuSize()-1),Index(0,getlaSize()-1)));
      updateVRef(VParent(Index(0,getuSize()-1),Index(0,getlaSize()-1)));
      updatelaRef(laParent(0,laSize-1));
      updategdRef(gdParent(0,gdSize-1));
      updateresRef(resParent(0,laSize-1));
      updaterFactorRef(rFactorParent(0,rFactorSize-1));

    }
    updategd(t);

    updateT(t); 
    updateh(t); 
    updateM(t); 
    facLLM(); 
    updateW(t); 
    updateV(t); 
    updateG(t); 
  }

  void MultiBodySystem::updaterFactors() {
    // UPDATERFACTORS updates r-factors for children
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

      cout << "Global r-Factor strategy not currently not available." << endl;
      throw 5;
    }
    else if(strategy == local) {
      Group::updaterFactors();
    } 
    else {
      cout << "Unknown strategy" << endl;
      throw 5;
    }
  }

  void MultiBodySystem::decreaserFactors() {

    for(vector<Link*>::iterator i = linkSetValuedActive.begin(); i != linkSetValuedActive.end(); ++i)
      (*i)->decreaserFactors();
  }

  void MultiBodySystem::updateM(double t) {
    M.init(0);
    Group::updateM(t);
  }

  void MultiBodySystem::updateh(double t) {
    h.init(0);
    Group::updateh(t);
  }

  void MultiBodySystem::updateW(double t) {
    W.init(0);
    Group::updateW(t);
  }

  void MultiBodySystem::updateV(double t) {
    V = W;
    Group::updateV(t);
  }

  void MultiBodySystem::updateG(double t) {

    G.resize() = SqrMat(trans(W)*slvLLFac(LLM,V)); 

    if(checkGSize) Gs.resize();
    else if(Gs.cols() != G.size()) {
      static double facSizeGs = 1;
      if(G.size()>limitGSize && facSizeGs == 1) facSizeGs = double(countElements(G))/double(G.size()*G.size())*1.5;
      Gs.resize(G.size(),G.size(),int(G.size()*G.size()*facSizeGs));
    }
    Gs << G;
  }

  void MultiBodySystem::updatewb(double t) {
    wb.init(0);
    Group::updatewb(t);
  }

  void MultiBodySystem::updater(double t) {
    r.init(0);
    Group::updater(t);
  }

  Vec MultiBodySystem::deltax(const Vec &zParent, double t, double dt) {
    if(q()!=zParent()) {
      updatezRef(zParent);
    }
    updatedx(t,dt);
    return xd;
  }

  Vec MultiBodySystem::deltaq(const Vec &zParent, double t, double dt) {
    // DELTAQ updates the position gap for the multibody system
    // INPUT 	zParent current state
    //			t		current time
    //			dt		time step

    if(q()!=zParent()) updatezRef(zParent);
    updatedq(t,dt);

    return qd;
  }

  Vec MultiBodySystem::deltau(const Vec &zParent, double t, double dt) {
    // DELTAU updates the velocity gap for the multibody system
    // INPUT 	zParent current state
    //			t		current time
    //			dt		time step

    if(q()!=zParent()) updatezRef(zParent);

    // TODO update auslagern
    updater(t); 
    updatedu(t,dt);
    // cout <<"Zeit : " << t  <<" "<< ud << endl;
    return ud;
  }

  void MultiBodySystem::save(const string &path, ofstream& outputfile) {
    Group::save(path,outputfile);
    outputfile << "# Acceleration of gravity:" << endl;
    outputfile << grav << endl << endl;;
  }

  void MultiBodySystem::load(const string &path, ifstream& inputfile) {

    setMultiBodySystem(this);

    Group::load(path, inputfile);

    string dummy;

    getline(inputfile,dummy); // #  Acceleration of gravity:
    inputfile >> grav;
    getline(inputfile,dummy); // # Rest of line
    getline(inputfile,dummy); // # Newline
  }

  void MultiBodySystem::save(const string &path, MultiBodySystem* mbs) {

    string model = path + "/" + mbs->getName() + ".mdl";

    ofstream outputfile(model.c_str(), ios::binary);

    mbs->save(path, outputfile);

    outputfile.close();
  }

  MultiBodySystem* MultiBodySystem::load(const string &path) {
    DIR* dir = opendir(path.c_str());
    dirent *first;
    first = readdir(dir); // .
    first = readdir(dir); // ..
    string name;
    while(first){
      first = readdir(dir);
      if(first) {
	name= first->d_name;
	unsigned int s = name.rfind(".mdl");
	if(s<name.size()) {
	  string buf = name.substr(0,s);
	  if(buf.find(".")>buf.size())
	    break;
	}
      }
    }
    closedir(dir);

    string model = path + "/" + name;

    ifstream inputfile(model.c_str(), ios::binary);

    MultiBodySystem* mbs = new MultiBodySystem("NoName");

    mbs->load(path, inputfile);

    inputfile.close();

    return mbs;
  }

  void MultiBodySystem::computeConstraintForces(double t) {
    //la = slvLU(G, -(trans(W)*slvLLFac(LLM,h) + wb));
    la = slvLS(G, -(trans(W)*slvLLFac(LLM,h) + wb));
  }

  void MultiBodySystem::projectViolatedConstraints(double t) {
    // PROJECTVIOLATEDCONSTRAINTS projects state, such that constraints are not violated

    if(laSize) {
      Vec nu(uSize);
      int gASize = 0;
      for(unsigned int i = 0; i<linkSetValuedActive.size(); i++) gASize += linkSetValuedActive[i]->getgSize();
      SqrMat Gv(gASize,NONINIT);
      Mat Wv(W.rows(),gASize,NONINIT);
      Vec gv(gASize,NONINIT);
      int gAIndi = 0;
      for(unsigned int i = 0; i<linkSetValuedActive.size(); i++) {
	Index I1 = Index(linkSetValuedActive[i]->getlaInd(),linkSetValuedActive[i]->getlaInd()+linkSetValuedActive[i]->getgSize()-1);
	Index Iv = Index(gAIndi,gAIndi+linkSetValuedActive[i]->getgSize()-1);
	Wv(Index(0,Wv.rows()-1),Iv) = W(Index(0,W.rows()-1),I1);
	gv(Iv) = g(linkSetValuedActive[i]->getgIndex());

	Gv(Iv,Iv) = G(I1,I1);
	int gAIndj = 0;
	for(unsigned int j = 0; j<i; j++) {
	  Index Jv = Index(gAIndj,gAIndj+linkSetValuedActive[j]->getgSize()-1);
	  Index J1 = Index(linkSetValuedActive[j]->getlaInd(),linkSetValuedActive[j]->getlaInd()+linkSetValuedActive[j]->getgSize()-1);
	  Gv(Jv,Iv) = G(J1,I1);
	  gAIndj+=linkSetValuedActive[j]->getgSize();
	}
	gAIndi+=linkSetValuedActive[i]->getgSize();
      }
      while(nrmInf(gv) >= 1e-8) {
	Vec mu = slvLU(Gv, -gv+trans(Wv)*nu);
	Vec dnu = slvLLFac(LLM,Wv*mu- M*nu);
	nu += dnu;
	q += T*dnu;
	updateKinematics(t);
	updateg(t);
	int gAIndi = 0;
	for(unsigned int i = 0; i<linkSetValuedActive.size(); i++) {
	  Index I1 = Index(linkSetValuedActive[i]->getlaInd(),linkSetValuedActive[i]->getlaInd()+linkSetValuedActive[i]->getgSize()-1);
	  Index Iv = Index(gAIndi,gAIndi+linkSetValuedActive[i]->getgSize()-1);
	  gv(Iv) = g(linkSetValuedActive[i]->getgIndex());
	  gAIndi+=linkSetValuedActive[i]->getgSize();
	}
      }
    }
  }

  // TODO auch in Subsystem
  void MultiBodySystem::savela() {
    for(vector<Link*>::iterator i = linkSetValuedActive.begin(); i != linkSetValuedActive.end(); ++i) 
      (**i).savela();
  }

  // TODO auch in Subsystem
  void MultiBodySystem::initla() {
    for(vector<Link*>::iterator i = linkSetValuedActive.begin(); i != linkSetValuedActive.end(); ++i) 
      (**i).initla();
  }

  void MultiBodySystem::preInteg(MultiBodySystem *parent) {
    if(preIntegrator){
      setProjectDirectory(name+".preInteg");
      setAccelerationOfGravity(parent->getGrav()); //TODO bedeutet, dass fuer Vorintegration der gravitationsvektor im MBS parent schon gesetzt sein muss.
      cout << "Initialisation of " << name << " for Preintegration..."<<endl;
      init();  
      cout << "Preintegration..."<<endl;
      preIntegrator->integrate(*this);
      closePlotFiles();
      writez();
      delete preIntegrator;
      preIntegrator=NULL; 
      cout << "Finished." << endl;
    }  
  }

  void MultiBodySystem::writez(){
    for(unsigned int i=0; i<object.size(); i++)  {
      object[i]->writeq();
      object[i]->writeu();
      object[i]->writex();
    }
    for(unsigned int i=0; i<EDI.size(); i++)  {
      EDI[i]->writex();
    }
  }

  void MultiBodySystem::readz0(){
    for(unsigned int i=0; i<object.size(); i++)  {
      object[i]->readq0();
      object[i]->readu0();
      object[i]->readx0();
    }
    for(unsigned int i=0; i<EDI.size(); i++)  {
      EDI[i]->readx0();
    }
  }

  void MultiBodySystem::addElement(Element *element_) {
    Object* object_=dynamic_cast<Object*>(element_);
    Link* link_=dynamic_cast<Link*>(element_);
    ExtraDynamicInterface* edi_=dynamic_cast<ExtraDynamicInterface*>(element_);
    if(object_) addObject(object_);
    else if(link_) addLink(link_);
    else if(edi_) addEDI(edi_);
    else{ cout << "Error: MultiBodySystem: addElement(): No such type of Element to add!"<<endl; throw 50;}
  }

  Element* MultiBodySystem::getElement(const string &name) {
 //   // GETELEMENT returns element
 //   unsigned int i1;
 //   for(i1=0; i1<object.size(); i1++) {
 //     if(object[i1]->getName() == name) return (Element*)object[i1];
 //   }
 //   for(i1=0; i1<object.size(); i1++) {
 //     if(object[i1]->getFullName() == name) return (Element*)object[i1];
 //   }
 //   unsigned int i2;
 //   for(i2=0; i2<link.size(); i2++) {
 //     if(link[i2]->getName() == name) return (Element*)link[i2];
 //   }
 //   for(i2=0; i2<link.size(); i2++) {
 //     if(link[i2]->getFullName() == name) return (Element*)link[i2];
 //   }
 //   unsigned int i3;
 //   for(i3=0; i3<EDI.size(); i3++) {
 //     if(EDI[i3]->getName() == name) return (Element*)EDI[i3];
 //   }
 //   for(i3=0; i3<EDI.size(); i3++) {
 //     if(EDI[i3]->getFullName() == name) return (Element*)EDI[i3];
 //   }
 //   if(!(i1<object.size())||!(i2<link.size())||!(i3<EDI.size())) cout << "Error: The MultiBodySystem " << this->name <<" comprises no element " << name << "!" << endl; 
 //   assert(i1<object.size()||i2<link.size()||!(i3<EDI.size()));
    return NULL;
  }

  int MultiBodySystem::solveConstraintsLinearEquations() {
    // SOLVELINEAREQUATIONS solves constraint equations with Cholesky decomposition
    la = slvLU(G,-(trans(W)*slvLLFac(LLM,h) + wb));
    return 1;
  }

  int MultiBodySystem::solveImpactsLinearEquations(double dt) {
    // SOLVELINEAREQUATIONS solves constraint equations with Cholesky decomposition
    la = slvLU(G,-(gd + trans(W)*slvLLFac(LLM,h)*dt));
    return 1;
  }

  int MultiBodySystem::solveConstraintsGaussSeidel() {
    // SOLVEGAUSSSEIDEL solves constraint equations with Gauss-Seidel scheme
    b.resize() = trans(W)*slvLLFac(LLM,h) + wb;

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

  int MultiBodySystem::solveImpactsGaussSeidel(double dt) {
    // SOLVEGAUSSSEIDEL solves constraint equations with Gauss-Seidel scheme
    b.resize() = gd + trans(W)*slvLLFac(LLM,h)*dt;

    checkImpactsForTermination();
    if(term) return 0 ;

    int iter;
    int checkTermLevel = 0;

    for(iter = 1; iter<=maxIter; iter++) {
      Group::solveImpactsGaussSeidel();
      if(checkTermLevel >= checkTermLevels.size() || iter > checkTermLevels(checkTermLevel)) {
	checkTermLevel++;
	checkImpactsForTermination();
	if(term) break;
      }
    }
    return iter;
  }

  int MultiBodySystem::solveConstraintsFixpointSingle() {
    // SOLVEFIXEDPOINTSINGLE solves constraint equations with single step iteration

    updaterFactors();

    b.resize() = trans(W)*slvLLFac(LLM,h) + wb;

    checkConstraintsForTermination();
    if(term) return 0;

    int iter, level = 0;
    int checkTermLevel = 0;

    for(iter = 1; iter<=maxIter; iter++) {

      if(level < decreaseLevels.size() && iter > decreaseLevels(level)) {
	level++;
	decreaserFactors();
	cout <<endl<< "Warning: decreasing r-factors at iter = " << iter << endl;
	if(warnLevel>=2) cout <<endl<< "Warning: decreasing r-factors at iter = " << iter << endl;
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


  int MultiBodySystem::solveImpactsFixpointSingle(double dt) {
    // SOLVEFIXEDPOINTSINGLE solves constraint equations with single step iteration

    updaterFactors();

    b.resize() = gd + trans(W)*slvLLFac(LLM,h)*dt;

    checkImpactsForTermination();
    if(term) return 0;

    int iter, level = 0;
    int checkTermLevel = 0;

    for(iter = 1; iter<=maxIter; iter++) {

      if(level < decreaseLevels.size() && iter > decreaseLevels(level)) {
	level++;
	decreaserFactors();
	cout <<endl<< "Warning: decreasing r-factors at iter = " << iter << endl;
	if(warnLevel>=2) cout <<endl<< "Warning: decreasing r-factors at iter = " << iter << endl;
      }

      Group::solveImpactsFixpointSingle();

      if(checkTermLevel >= checkTermLevels.size() || iter > checkTermLevels(checkTermLevel)) {
	checkTermLevel++;
	checkImpactsForTermination();
	if(term) break;
      }
    }
    return iter;
  }

  int MultiBodySystem::solveConstraintsRootFinding() {
    // SOLVEROOTFINDING solves constraint equations with general Newton method

    updaterFactors();

    b.resize() = trans(W)*slvLLFac(LLM,h) + wb;

    int iter;
    //    int prim = 0;
    int checkTermLevel = 0;

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
	double dx, xj;

	for(int j=0; j<la.size(); j++) {
	  xj = la(j);

	  dx = (epsroot() * 0.5);
	  do dx += dx;
	  while (xj + dx == la(j));

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
	dx >> slvLL(J,trans(Jprox)*res0);
      }
      else if(linAlg == PseudoInverse) dx >> slvLS(Jprox,res0);
      else throw 5;

      double alpha = 1;       

      Vec La_old = la.copy();

      double nrmf = 1;
      for (int k=0; k<maxDampingSteps; k++) {
	la = La_old - alpha*dx;
	solveConstraintsRootFinding();
	nrmf = nrm2(res);
	if(nrmf < nrmf0) break;

	alpha = 0.5*alpha;  
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

  int MultiBodySystem::solveImpactsRootFinding(double dt) {
    // SOLVEROOTFINDING solves constraint equations with general Newton method

    updaterFactors();

    b.resize() = gd + trans(W)*slvLLFac(LLM,h)*dt;

    int iter;
    //    int prim = 0;
    int checkTermLevel = 0;

    Group::solveImpactsRootFinding(); 
    double nrmf0 = nrm2(res);
    Vec res0 = res.copy();

    checkImpactsForTermination();
    if(term)
      return 0 ;

    DiagMat I(la.size(),INIT,1);
    for(iter=1; iter<maxIter; iter++) {

      if(Jprox.size() != la.size()) Jprox.resize(la.size(),NONINIT);

      if(numJac) {
	double dx, xj;

	for(int j=0; j<la.size(); j++) {
	  xj = la(j);

	  dx = (epsroot() * 0.5);
	  do dx += dx;
	  while (xj + dx == la(j));

	  la(j) += dx;
	  Group::solveImpactsRootFinding(); 
	  la(j) = xj;
	  Jprox.col(j) = (res-res0)/dx;
	}
      } 
      else jacobianImpacts();
      Vec dx;
      if(linAlg == LUDecomposition) dx >> slvLU(Jprox,res0);
      else if(linAlg == LevenbergMarquardt) {
	SymMat J = SymMat(JTJ(Jprox) + lmParm*I);
	dx >> slvLL(J,trans(Jprox)*res0);
      }
      else if(linAlg == PseudoInverse) dx >> slvLS(Jprox,res0);
      else throw 5;

      double alpha = 1;       

      Vec La_old = la.copy();

      double nrmf = 1;
      for (int k=0; k<maxDampingSteps; k++) {
	la = La_old - alpha*dx;
	solveImpactsRootFinding();
	nrmf = nrm2(res);
	if(nrmf < nrmf0) break;

	alpha = 0.5*alpha;  
      }
      nrmf0 = nrmf;
      res0 = res;

      if(checkTermLevel >= checkTermLevels.size() || iter > checkTermLevels(checkTermLevel)) {
	checkTermLevel++;
	checkImpactsForTermination();
	if(term) break;
      }
    }
    return iter;
  }
  void MultiBodySystem::checkConstraintsForTermination() {

    term = true;
    for(vector<Subsystem*>::iterator i = subsystem.begin(); i != subsystem.end(); ++i) { 
      (*i)->checkConstraintsForTermination(); 
      if(term == false) return;
    }

    for(vector<Link*>::iterator i = linkSetValuedActive.begin(); i != linkSetValuedActive.end(); ++i) {
      (**i).checkConstraintsForTermination();
      if(term == false) return;
    }
  }

  void MultiBodySystem::checkImpactsForTermination() {

    term = true;
    for(vector<Subsystem*>::iterator i = subsystem.begin(); i != subsystem.end(); ++i) {
      (*i)->checkImpactsForTermination(); 
      if(term == false) return;
    }

    for(vector<Link*>::iterator i = linkSetValuedActive.begin(); i != linkSetValuedActive.end(); ++i) {
      (**i).checkImpactsForTermination();
      if(term == false) return;
    }
  }

  int MultiBodySystem::solveConstraints() {
    // SOLVE solves prox-functions depending on solver settings
    // INPUT t	time

    if(la.size()==0) return 0;

    if(useOldla)initla();
    else la.init(0);

    int iter;
    Vec laOld;
    laOld = la;
    iter = (this->*solveConstraints_)(); // solver election
    if(iter >= maxIter) {
      cout << endl;
      cout << "Iterations: " << iter << endl;
      cout << "\nError: no convergence."<< endl;
      if(stopIfNoConvergence) {
	if(dropContactInfo) dropContactMatrices();
	assert(iter < maxIter);
      }
      cout << "Anyway, continuing integration..."<< endl;
    }

    if(warnLevel>=1 && iter>highIter)
      cerr <<endl<< "Warning: high number of iterations: " << iter << endl;

    if(useOldla) savela();

    return iter;
  }

  int MultiBodySystem::solveImpacts(double dt) {
    // SOLVE solves prox-functions depending on solver settings
    // INPUT t	time

    if(la.size()==0) return 0;

    if(useOldla)initla();
    else la.init(0);

    int iter;
    Vec laOld;
    laOld = la;
    iter = (this->*solveImpacts_)(dt); // solver election
    if(iter >= maxIter) {
      cout << endl;
      cout << "Iterations: " << iter << endl;
      cout << "\nError: no convergence."<< endl;
      if(stopIfNoConvergence) {
	if(dropContactInfo) dropContactMatrices();
	assert(iter < maxIter);
      }
      cout << "Anyway, continuing integration..."<< endl;
    }

    if(warnLevel>=1 && iter>highIter)
      cerr <<endl<< "Warning: high number of iterations: " << iter << endl;

    if(useOldla) savela();

    return iter;
  }

  void MultiBodySystem::dropContactMatrices() {
    // DROPCONTACTMATRICES writes a file with relevant matrices for debugging

    cout << "dropping contact matrices to file <dump_matrices.asc>" << endl;
    ofstream contactDrop("dump_matrices.asc");   

    contactDrop << "constraint functions g" << endl << trans(g) << endl << endl;
    contactDrop << endl;
    contactDrop << "mass matrix M" << endl << M << endl << endl;
    contactDrop << "generalized force directions W" << endl << W << endl << endl;
    contactDrop << "Delassus matrix G" << endl << G << endl << endl;
    contactDrop << endl;
    contactDrop << "constraint velocities gp" << endl << trans(gd) << endl << endl;
    contactDrop << "non-holonomic part in gp; b" << endl << trans(b) << endl << endl;
    contactDrop << "Lagrange multipliers la" << endl << trans(la) << endl << endl;
    contactDrop.close();
  }

  string MultiBodySystem::getSolverInfo() {
    // GETSOLVERINFO returns solver information

    stringstream info;

    // Solver-Name
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

  void MultiBodySystem::initDataInterfaceBase() 
  {
    vector<Link*>::iterator il1;
    for(il1 = link.begin(); il1 != link.end(); ++il1) (*il1)->initDataInterfaceBase(this);
    vector<Object*>::iterator io1;
    for(io1 = object.begin(); io1 != object.end(); ++io1) (*io1)->initDataInterfaceBase(this);
    vector<ExtraDynamicInterface*>::iterator ie1;
    for(ie1 = EDI.begin(); ie1 != EDI.end(); ++ie1) (*ie1)->initDataInterfaceBase(this); 
  }

  CoordinateSystem* MultiBodySystem::findCoordinateSystem(const string &name) {

    istringstream stream(name);

    char dummy[10000];
    vector<string> l;
    do {
      stream.getline(dummy,10000,'.');
      l.push_back(dummy);
    } while(!stream.eof());

    if(l.size() == 1)
      throw 5;

    if(l.size() == 2)
      return getCoordinateSystem(l[1]);

    Subsystem *sys = this;
    for(unsigned int i=1; i<l.size()-2; i++) {
      sys = static_cast<Subsystem*>(sys->getSubsystem(l[i]));
    }
    return sys->getObject(l[l.size()-2])->getCoordinateSystem(l[l.size()-1]);
  }

  Contour* MultiBodySystem::findContour(const string &name) {

    istringstream stream(name);

    char dummy[10000];
    vector<string> l;
    do {
      stream.getline(dummy,10000,'.');
      l.push_back(dummy);
    } while(!stream.eof());

    if(l.size() == 1)
      throw 5;

    if(l.size() == 2)
      return getContour(l[1]);

    Subsystem *sys = this;
    for(unsigned int i=1; i<l.size()-2; i++) {
      sys = static_cast<Subsystem*>(sys->getSubsystem(l[i]));
    }
    return sys->getObject(l[l.size()-2])->getContour(l[l.size()-1]);
  }

  void MultiBodySystem::shift(Vec &zParent, const Vector<int> &jsv_, double t) {
    if(q()!=zParent()) {
      updatezRef(zParent);
    }
    jsv = jsv_;
    //cout <<endl<< "event at time t = " << t << endl<<endl;
    //cout<< "sv = " << trans(sv) << endl;
    //cout << "jsv = "<< trans(jsv) << endl;

    updateCondition(); // Entscheide, welche Bindungen dazukommen bzw. entfallen
    checkActiveLinks(); // Liste mit aktiven Links (g<=0) neu anlegen
    //cout <<"impact = "<< impact <<endl;
    //cout <<"sticking = "<< sticking <<endl;

    if(impact) {

      checkAllgd(); // Es müssen alle geschlossenen Kontakte berücksichtigt werden
      calcgdSize(); // 
      updategdRef(gdParent(0,gdSize-1));
      calclaSize();
      calcrFactorSize();
      setlaIndMBS(laInd);
      updateWRef(WParent(Index(0,getuSize()-1),Index(0,getlaSize()-1)));
      updateVRef(VParent(Index(0,getuSize()-1),Index(0,getlaSize()-1)));
      updatelaRef(laParent(0,laSize-1));
      updaterFactorRef(rFactorParent(0,rFactorSize-1));
      //cout << "laSize before impact = " << laSize << " gdSize = " << gdSize <<endl;

      updateKinematics(t); // prüfen, ob nötig
      updateg(t); // prüfen, ob nötig
      updategd(t); // wichtig wegen updategdRef
      //updateh(t); // h-Vektor geht nicht in Stoß ein
      updateW(t); // wichtig wegen updateWRef
      updateV(t); // ..
      updateG(t); // ..

      int iter;
      iter = solveImpacts();
      //cout <<"Iterations = "<< iter << endl;
      u += deltau(zParent,t,0);

      //saveActive();
      checkActivegdn(); // Prüfen welche Kontakte nach Stoß aufgehen bzw. geschlossen bleiben
      checkActiveLinks(); // Liste mit aktiven Links (g<=0) neu anlegen

      //calcgdSize(); Darf nicht aufgerufen werden
      //updategdRef(); Darf nicht aufgerufen werden

      calclaSize();
      calcrFactorSize();
      setlaIndMBS(laInd);
      updateWRef(WParent(Index(0,getuSize()-1),Index(0,getlaSize()-1)));
      updateVRef(VParent(Index(0,getuSize()-1),Index(0,getlaSize()-1)));
      updatelaRef(laParent(0,laSize-1));
      updatewbRef(wbParent(0,laSize-1));
      updaterFactorRef(rFactorParent(0,rFactorSize-1));
      //cout << "laSize wrt gd (impact) = " << laSize<< " gdSize = " << gdSize <<endl;

      if(laSize) {

	updateKinematics(t); // Nötig da Geschwindigkeitsänderung
	// updateg(t); Unnötig, da keine Lageänderung
	updategd(t); // Nötig da Geschwindigkeitsänderung
	updateh(t); 
	updateW(t); 
	updateV(t); 
	updateG(t); 
	updatewb(t); 
	int iter;
	iter = solveConstraints();
	checkActivegdd();
	checkActiveLinks();
	calclaSize();
	calcrFactorSize();
	setlaIndMBS(laInd);
	updateWRef(WParent(Index(0,getuSize()-1),Index(0,getlaSize()-1)));
	updateVRef(VParent(Index(0,getuSize()-1),Index(0,getlaSize()-1)));
	updatelaRef(laParent(0,laSize-1));
	updatewbRef(wbParent(0,laSize-1));
	updaterFactorRef(rFactorParent(0,rFactorSize-1));
	//cout << "laSize wrt gdd (impact) = " << laSize<< " " << gdSize << endl;
      }
    } 
    else if(sticking) {

      calclaSize();
      calcrFactorSize();
      setlaIndMBS(laInd);
      updateWRef(WParent(Index(0,getuSize()-1),Index(0,getlaSize()-1)));
      updateVRef(VParent(Index(0,getuSize()-1),Index(0,getlaSize()-1)));
      updatelaRef(laParent(0,laSize-1));
      updatewbRef(wbParent(0,laSize-1));
      updaterFactorRef(rFactorParent(0,rFactorSize-1));
      //cout << " laSize before sticking = " << laSize << " gdSize = " << gdSize <<endl;

      if(laSize) {

	updateKinematics(t); // Prüfen ob nötig
	updateg(t); // Prüfen ob nötig
	updategd(t); // Prüfen ob nötig
	updateT(t);  // Prüfen ob nötig
	updateh(t);  // Prüfen ob nötig
	updateM(t);  // Prüfen ob nötig
	facLLM();  // Prüfen ob nötig
	updateW(t);  // Prüfen ob nötig
	updateV(t);  // Prüfen ob nötig
	updateG(t);  // Prüfen ob nötig
	updatewb(t);  // Prüfen ob nötig
	int iter;
	iter = solveConstraints();
	checkActivegdd();
	checkActiveLinks();
	calclaSize();
	calcrFactorSize();
	setlaIndMBS(laInd);
	updateWRef(WParent(Index(0,getuSize()-1),Index(0,getlaSize()-1)));
	updateVRef(VParent(Index(0,getuSize()-1),Index(0,getlaSize()-1)));
	updatelaRef(laParent(0,laSize-1));
	updatewbRef(wbParent(0,laSize-1));
	updaterFactorRef(rFactorParent(0,rFactorSize-1));
	//cout << "laSize wrt gdd (Sticking) = " << laSize<< " " << gdSize << endl;
      }
    } 
    else {

      checkActiveLinks();
      calclaSize();
      calcrFactorSize();
      setlaIndMBS(laInd);
      updateWRef(WParent(Index(0,getuSize()-1),Index(0,getlaSize()-1)));
      updateVRef(VParent(Index(0,getuSize()-1),Index(0,getlaSize()-1)));
      updatelaRef(laParent(0,laSize-1));
      updatewbRef(wbParent(0,laSize-1));
      updaterFactorRef(rFactorParent(0,rFactorSize-1));
      //cout << "laSize wrt gdd (smooth) = " << laSize<< " " << gdSize << endl;
    }
    impact = false;
    sticking = false;
  }
}
