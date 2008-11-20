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

#ifndef _MULTI_BODY_SYSTEM_H_
#define _MULTI_BODY_SYSTEM_H_

#include <string>
#include "group.h"
#include "sparse_matrix.h"

using namespace std;

namespace MBSim {

  class CoordinateSystem;
  class HydFluid;
  class ExtraDynamicInterface;
  class DataInterfaceBase;

  class Integrator;

  /* Solver for contact parameter identification */
  enum Solver {FixedPointTotal, FixedPointSingle, GaussSeidel, LinearEquations, RootFinding};
  /* r-Factor strategies */
  enum Strategy {global, local};
  /* Linear algebra for root-function solver solveN */
  enum LinAlg {LUDecomposition,LevenbergMarquardt,PseudoInverse};

  /*! Comment
   *
   * */
  class MultiBodySystem : public Group {


    protected:

      vector<Object*>                objects2plot;
      vector<Link*>                  links2plot;
      vector<Contour*>               contours2plot;
      vector<CoordinateSystem*>                  ports2plot;
      vector<ExtraDynamicInterface*> EDIs2plot;

      bool term;
      Matrix<Sparse, double> Gs;
      SqrMat Jprox;

      SqrMat G;
      Vec b;

      SymMat MParent;
      SymMat LLMParent;
      Mat TParent;
      Mat WParent;
      Mat VParent;
      Vec wbParent;
      Vec laParent;
      Vec rFactorParent;
      Vec sParent;
      Vec resParent;
      Vec gParent;
      Vec gdParent;
      Vec zdParent;
      Vec hParent;
      Vec rParent;
      Vec fParent;
      Vec svParent;
      Vector<int> jsvParent;
      /** gravitation common for all components */
      Vec grav;
      //bool activeConstraintsChanged;


      int maxIter, highIter, maxDampingSteps;
      double lmParm;
      int warnLevel;
      Solver contactSolver, impactSolver;
      Strategy strategy;
      LinAlg linAlg;
      bool stopIfNoConvergence;
      bool dropContactInfo;
      bool useOldla;
      bool numJac;
      Vector<int> decreaseLevels;
      Vector<int> checkTermLevels;

      bool checkGSize;
      int limitGSize;
      void updatezRef(const Vec &ext);
      void updatezdRef(const Vec &ext);

      void updaterFactors();
      void computeConstraintForces(double t);

      /* Name of directory where output is processed */
      string directoryName;
      /*! Test and enumerate directories for simulation output */
      void setDirectory();

      /* Hydraulic fluid, only for hydraulic systems */
      HydFluid *fl;
      /* Ambient pressure, only for hydraulic systems */
      double pinf;
      Integrator *preIntegrator;

      bool peds, impact, sticking;

    public:
      const Vec& getAccelerationOfGravity() const {return grav;}

      void setImpact(bool impact_) {impact = impact_;}
      void setSticking(bool sticking_) {sticking = sticking_;}

      void computeInitialCondition();

      /*! Calls updater for children at time \param t */
      void updater(double t);
      /*! Calls updateh for children at time \param t */
      void updateh(double t);
      /*! Calls updateM for children at time \param t */
      void updateM(double t);
      /*! Calls updateW for children at time \param t */
      void updateW(double t);
      /*! Calls updateV for children at time \param t */
      void updateV(double t);
      void updateG(double t);
      void updatewb(double t);
      /*! Constructor */
      MultiBodySystem();
      /*! Constructor */
      MultiBodySystem(const string &projectName);
      /*! Destructor */
      ~MultiBodySystem();
      /*! Adds \param mbs to multibody system */

      void init();
      //void checkActiveConstraints();
      //void setActiveConstraintsChanged(bool b) {activeConstraintsChanged = b;}
      virtual void preInteg(MultiBodySystem *parent);
      /*! Projects state at time \param t, such that constraints are not violated */
      void projectViolatedConstraints(double t);
      /*! Return vector of gravitational acceleration \return g in world system */
      const Vec& getGrav() const {return grav;};
      /*! Define vector of gravitational acceleration \param g in world system */
      void setAccelerationOfGravity(const Vec& g);
      /*! 
       * Define \param directoryName_ for simulation output.
       * Default is Element::(fullName+i) with i used for enumeration 
       */
      void setProjectDirectory(const string &directoryName_) {directoryName = directoryName_;}
      void setPreInteg(Integrator *preInteg_) {preIntegrator = preInteg_;}
      const string& getDirectoryName() {return dirName;}
      string getFullName() const {return name;}

      // Implementation of the ODERootFindingSystemInterface (Integratores)
      void zdot(const Vec& z, Vec& zd, double t);
      Vec zdot(const Vec& z, double t);
      void getsv(const Vec&, Vec&, double);
      virtual void shift(Vec& z, const Vector<int>& jsv, double t);

      void plot(const Vec& z, double t, double dt=1);
      /* Updates the position depending structures for multibody system */
      void update(const Vec &zParent, double t);
      /*! Computes velocity difference for current time \param t with state \param zParent and time step \param dt */
      Vec deltau(const Vec &uParent, double t, double dt);
      /*! Updates position gap for current time \param t with state \param zParent and time step \param dt*/
      Vec deltaq(const Vec &zParent, double t, double dt);
      Vec deltax(const Vec &zParent, double t, double dt);
      void decreaserFactors();
      /*! Initialises the state of objects and EDIs */
      void initz(Vec& z0);
      Vec& getsv() {return sv;}
      const Vec& getsv() const {return sv;}
      Vector<int>& getjsv() {return jsv;}
      const Vector<int>& getjsv() const {return jsv;}

      bool driftCompensation(Vec& z, double t) { return false; }

      void savela();
      void initla();

      using Subsystem::plot;

      /*! Compute kinetic energy of entire multibodysystem, which is the quadratic form \f$\frac{1}{2}\boldsymbol{u}^T\boldsymbol{M}\boldsymbol{u}\f$ for all systems */
      double computeKineticEnergy() { return 0.5*trans(u)*M*u; }
      /* ! Compute potential energy of entire multibody system */
      double computePotentialEnergy();

      /* Method to add any element \param element_ (Link,Object,ExtraDynamicInterface) by dynamic casting */
      void addElement(Element *element_);
      /* Returns the pointer to an element \param name */
      Element* getElement(const string &name); 

      const Matrix<Sparse, double>& getGs() const {return Gs;}
      Matrix<Sparse, double>& getGs() {return Gs;}
      const SqrMat& getG() const {return G;}
      SqrMat& getG() {return G;}
      const Vec& getb() const {return b;}
      Vec& getb() {return b;}
      const SqrMat& getJprox() const {return Jprox;}
      SqrMat& getJprox() {return Jprox;}

      void setTermination(bool term_) {term = term_;}

      void setNumJacProj(bool numJac_) {numJac = numJac_;}
      void setMaxIter(int iter) {maxIter = iter;}
      void setHighIter(int iter) {highIter = iter;}
      void setMaxDampingSteps(int maxDSteps) {maxDampingSteps = maxDSteps;}
      void setLevenbergMarquardtParam(double lmParm_) {lmParm = lmParm_;}
      /*! Set Solver for treatment of constraint problems */
      void setConstraintSolver(Solver solver_) {contactSolver = solver_;}                         
      void setImpactSolver(Solver solver_) {impactSolver = solver_;}                         
      void setLinAlg(LinAlg linAlg_) {linAlg = linAlg_;}                         
      void setStrategy(Strategy strategy_) {strategy = strategy_;}
      /*! Returns information \return string for solver including strategy and linear algebra */
      string getSolverInfo();

      /*! 
       * Specify whether time integration should be stopped \param flag(true) in case of no convergence of constraint-problem
       * or only a warning should be processed (false=default)
       * \param dropInfo is used for dropping contact informations to file (true)
       */
      void setStopIfNoConvergence(bool flag, bool dropInfo = false) {stopIfNoConvergence = flag; dropContactInfo=dropInfo;}
      /*! Writes a file with relevant matrices for debugging */
      void dropContactMatrices();
      void setUseOldla(bool flag) {useOldla = flag;}
      void setDecreaseLevels(const Vector<int> &decreaseLevels_) {decreaseLevels = decreaseLevels_;}
      void setCheckTermLevels(const Vector<int> &checkTermLevels_) {checkTermLevels = checkTermLevels_;}

      void setCheckGSize(bool checkGSize_) {checkGSize = checkGSize_;}
      void setLimitGSize(int limitGSize_) {limitGSize = limitGSize_; checkGSize = false;}

      //MultiBodySystem* getMultiBodySystem() const {return mbs;};

      /*! Solves prox-functions at time \param t depending on solver settings */
      virtual int solveConstraints(); 
      virtual int solveImpacts(double dt = 0); 
      int (MultiBodySystem::*solveConstraints_)();
      int (MultiBodySystem::*solveImpacts_)(double dt);

      int solveConstraintsGaussSeidel();
      int solveImpactsGaussSeidel(double dt = 0);

      int solveConstraintsFixpointSingle(); 
      int solveImpactsFixpointSingle(double dt = 0); 

      int solveConstraintsRootFinding(); 
      int solveImpactsRootFinding(double dt = 0); 

      int solveConstraintsLinearEquations(); 
      int solveImpactsLinearEquations(double dt = 0); 

      void checkConstraintsForTermination(); 
      void checkImpactsForTermination(); 

      // Just for testing
      void setPartialEventDrivenSolver(bool peds_) {peds = peds_;}

      void writez();
      void readz0();

      /*! get ambient pressure
       *  \return pinf
       * */
      const double getpinf() const {return pinf;};
      /*! set ambient pressure
       * \param pinf_
       * */
      void setpinf(const double pinf_) {pinf=pinf_;};
      /*!
       *  set hydraulic fluid for system
       *  \param fl_
       * */
      void setFluid(HydFluid *fl_) {fl=fl_;}
      /*! get hydraulic fluid of system
       *  \return fl
       * */
      HydFluid *getFluid(){return fl;}  

      void initDataInterfaceBase();

      string getType() const {return "MultiBodySystem";}

      CoordinateSystem* findCoordinateSystem(const string &name);
      Contour* findContour(const string &name);
      MultiBodySystem* getMultiBodySystem() {return this;}
      void load(const string &path, ifstream& inputfile);
      void save(const string &path, ofstream& outputfile);
      static MultiBodySystem* load(const string &path);
      static void save(const string &path, MultiBodySystem* mbs);
  };

}

#endif
