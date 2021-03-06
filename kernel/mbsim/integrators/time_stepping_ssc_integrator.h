/* Copyright (C) 2004-2009  MBSim Development Team

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
 *   rhuber@users.berlios.de
 */

#ifndef _TIME_STEPPING_SSC_INTEGRATOR_H_ 
#define _TIME_STEPPING_SSC_INTEGRATOR_H_

#include "integrator.h"
#include "mbsim/utils/stopwatch.h"

namespace MBSim {
  class Link;
}

namespace MBSimIntegrator {

  /** \brief Half-explicit time-stepping integrator of first or higer order with StepSize Control (SSC)
   *  important options / settings :      
   *                      
   *  a) setMaxOrder(int order, int method=0)
   *                    order:   maximum order of integration scheme 
   *                              1 to 3 with SSC by extrapolation (method=0) (order=4 without SSC)
   *                              1 to 4 with embedded SSC (1=1(2), ... 4=4(5)) (method=1 or 2)
   *                    method:  method used for error estimation and step size control
   *                              0: step size control by extrapolation (steps wit dt and dt/2 are compared)  DEFAULT
   *                              1: embedded method (compare maxOrder maxOrder+1); proceed with maxOrder (recommended)
   *                              2: embedded method with local extrapolation (integration is continued with maxOrder+1)
   *
   * b) setFlagErrorTest(int Flag, bool alwaysValid= true)
   *                    Flag:    for scaling variables for the purpose of error estimation  
   *                              0: include velocities u for error test
   *                              2: scale velocities u with stepsize for error test
   *                              3: exclude velocities u for error test
   *            alwaysValid:  true : u is scaled resp. exluded during smooth and nonsmooth steps
   *            alwaysValid: false : u is scaled resp. exluded only during nonsmooth steps
   *
   * c) deactivateSSC(bool flag=false) : maximum order: 1 to 4
   *
   * d) setGapControl(bool FlagGapControl=true, int GapControlStrategy=0)                  
   *        FlagGapControl       activate / deactivate gap control
   *        GapControlStrategy   choose strategy for Gap Control (1:maximal Stepsize to  4:minimal Penetration) 
   *                              1: uses biggest root (maximal dt)
   *                              2: score for all roots are evaluated
   *                              3: gapTol is used
   *                              4: uses smallest root (minimal penetration)
   *                              0: gap control deactivated
   *
   *  \author Robert Huber
   *  \date 2009-09-07 modifications for mbsim_dev
   */
  class TimeSteppingSSCIntegrator : public Integrator { 

    protected:
      MBSim::DynamicSystemSolver* sysT1;
      MBSim::DynamicSystemSolver* sysT2;
      MBSim::DynamicSystemSolver* sysT3;

      double dt, dtOld, dte;
      double dtMin, dtMax;
      double dt_SSC_vorGapControl;
      bool driftCompensation;
      double t, tPlot;
      int qSize, xSize, uSize, zSize;
      fmatvec::Vec ze, zi, zT1, zT2, zT3, z1d, z2d, z2dRE, z3d, z4d, z6d, z2b, z3b, z4b, z6b, zStern;
      fmatvec::VecInt LS, LSe, LStmp_T1, LStmp_T2, LStmp_T3, LSA, LSB1, LSB2, LSC1, LSC2, LSC3, LSC4, LSD1, LSD2, LSD3, LSD4, LSD5, LSD6;
      fmatvec::Vec la, lae, la1d, la2b;
      fmatvec::VecInt laSizes, laeSizes, la1dSizes, la2bSizes;
      fmatvec::Vec qT1, qT2, qT3;
      fmatvec::Vec uT1, uT2, uT3;
      fmatvec::Vec xT1, xT2, xT3;
      fmatvec::Vec gInActive, gdInActive;
      std::vector<MBSim::Link*> SetValuedLinkListT1;
      std::vector<MBSim::Link*> SetValuedLinkListT2;
      std::vector<MBSim::Link*> SetValuedLinkListT3;

      int StepsWithUnchangedConstraints;

      /** include (0) or exclude (3) variable u or scale (2) with stepsize for error test*/
      int FlagErrorTest;
      /** FlagErrorTest is always valid or only during nonsmooth steps */
      bool FlagErrorTestAlwaysValid;
      /** Absolute Toleranz */
      fmatvec::Vec aTol;
      /** Relative Toleranz */
      fmatvec::Vec rTol;
      /** activate step size control*/
      bool FlagSSC;
      /** maximum order of integration scheme 
      *   SSC by extrapolation (steps with differnt dt are compared): 1, 2, 3  
      *   SSC with embedded methods (differnt orders are compared): 1 to 4  [ 1= 1(2), ... 4= 4(5) ] maxOrder(maxOrder+1) */
      int maxOrder;
      /** method for intrgration and step size control 
      *   method=0   step size control by extrapolation (steps wit dt and dt/2 are compared)  DEFAULT
      *   method=1   embedded method for step size control (maxOrder and maxOrder+1 are compared);  proceed with maxOrder (recommended)
      *   method=2   embedded method for SSC with -local extrapolation- (integration is continued with maxOrder+1) */
      int method;
      /* Flag for Gap Control */
      bool FlagGapControl;
      /** Toleranz for closing gaps */
      double gapTol;
      /** maximal gain factor for increasing dt by stepsize control (default 2.5; maxGain * safetyFactor must be GT 1)*/
      double maxGainSSC;
      /** safety factor for stepsize estimation: dt = dt_estimate * safetyFactorSSC (]0;1]; default 0.6)*/
      double safetyFactorSSC; 
      /** filestream for integrator info at each step */
      std::ofstream integPlot;
      /** Flag: write integrator info at each step to a file (default true)*/
      bool FlagPlotIntegrator;
      /** Flag: write integration summary to a file (default true)*/
      bool FlagPlotIntegrationSum;
      /** Flag: write integration info (integ. summary) to cout  (default true)*/
      bool FlagCoutInfo;
      /** Flag: write output info to cout only for plot-time-instances (default false)*/
      bool FlagOutputOnlyAtTPlot;
      /** every successful integration step is ploted (set dtPlot=0 to activate FlagPlotEveryStep) (default false)*/
      bool FlagPlotEveryStep;
      /** Flag interpolate z and la for plotting (default true)*/
      bool outputInterpolation;
      /** Safety factor for GapControl (dtNew = dt_gapcontrol*safetyFactor; default min(1+RTol,1.001) */
      double safetyFactorGapControl;
      /** choose strategy for Gap Control (1:maximal Stepsize to  4:minimal Penetration) 
       *   1: uses biggest root (maximal dt)
       *   2: score for all roots are evaluated
       *   3: gapTol is used
       *   4: uses smallest root (minimal penetration)
       *   0: gap control deactivated
       */
      int GapControlStrategy;
      /** Number of Threads */
      int numThreads;
      /** computaional time*/
      double time;
      /** for internal use (start clock, integration info ...) */
      MBSim::StopWatch Timer;
      int iter, iterA, iterB1, iterB2, iterC1, iterC2, iterC3, iterC4, iterB2RE, maxIterUsed, maxIter, sumIter;
      int integrationSteps, integrationStepswithChange, refusedSteps, refusedStepsWithImpact;
      int wrongAlertGapControl, stepsOkAfterGapControl, stepsRefusedAfterGapControl, statusGapControl;
      int singleStepsT1, singleStepsT2, singleStepsT3;
      double dtRelGapControl, qUncertaintyByExtrapolation;
      int indexLSException;
      fmatvec::Vec gUniActive;
      double Penetration, PenetrationCounter, PenetrationLog, PenetrationMin, PenetrationMax;
      double maxdtUsed, mindtUsed;
      bool ChangeByGapControl;      
      bool calcBlock2;
      bool IterConvergence;
      bool ConstraintsChanged, ConstraintsChangedBlock1, ConstraintsChangedBlock2;
      int integrationStepsOrder1;
      int integrationStepsOrder2;
      int order;
      int StepTrials;
      int AnzahlAktiverKontakte;
      double gNDurchschnittprostep;

    public:
      /*! Constructor with \default dt(1e-5), \default driftCompensation(false) */
      TimeSteppingSSCIntegrator();
      /*! Destructor */
      ~TimeSteppingSSCIntegrator();
      /*! Set initial step size */
      void setInitialStepSize(double dt_) {dt = dt_;}
      /*! Set maximal step size */
      void setStepSizeMax(double dtMax_) {dtMax = dtMax_;}
      /*! Set minimal step size (default 2*maxOrder*epsroot() */
      void setStepSizeMin(double dtMin_) {dtMin = dtMin_;}
      /*! Set maximal gain for increasing dt by stepsize control */
      void setmaxGainSSC(double maxGain) {maxGainSSC = maxGain;}
      /*! safety factor for stepsize estimation: dt = dt_estimate * safetyFactorSSC (]0;1]; default 0.6)*/
      void setSafetyFactorSSC(double sfactor) {safetyFactorSSC=sfactor;}
      void setSafetyFactorGapControl(double s){safetyFactorGapControl=s;}
      /*! Set Flag for output interpolation */
      void setOutputInterpolation(bool flag=true) {outputInterpolation = flag;}
      /*! set Flag for  writing integrator info at each step to a file (default true)*/
      void setFlagPlotIntegrator(bool flag=true) {FlagPlotIntegrator = flag;}
      /*! set Flag for writing output only at tPlot-Time instances or not (default false)*/
      void setFlagOutputOnlyAtTPlot(bool flag=true) {FlagOutputOnlyAtTPlot = flag;}
/*! Set Flag to optimise dt for minmal penetration of unilateral links;
       *  choose strategy for Gap Control (1:maximal Stepsize to  4:minimal Penetration) 
       *   1: uses biggest root (maximal dt)
       *   2: score for all roots are evaluated
       *   3: gapTol is used
       *   4: uses smallest root (minimal penetration)
       *   0: gap control deactivated with statistic calculations
       *   -1: gap control deactivated without statistic calculations
       */
      void setGapControl(int strategy=1) {FlagGapControl=(strategy>=0); GapControlStrategy=(strategy<0)?0:strategy; }
      /*! Set drift compensation */
      void setDriftCompensation(bool dc) {driftCompensation = dc;}
      /*! set maximum order (1,2,3 (method=0) or 1 to 4 (method=1,2) and
       *      method 0: SSC by extrapolation (recommended!!);  
       *      1,2: embedded SSC; proceed with maxOrder [1] (recommended if you don't want to use 0) or with maxOrder+1 [2]
       *      without SSC maximum order from 1 to 4 is possible*/  
      void setMaxOrder(int order_, int method_=0);
      /* deactivate step size control */
      void deactivateSSC(bool flag=false) {FlagSSC=flag;}
      /*! Set Flag vor ErrorTest (default 0: all variables ae tested;  2: u is scaled with dt;  3: exclude u
       *      alwaysValid = true  : u is scaled resp. exluded during the smooth and nonsmooth part
       *      alwaysValid = false : u is scaled resp. exluded only during nonsmooth steps
       */
      void setFlagErrorTest(int Flag, bool alwaysValid=true);

      /*! Start the integration */
      void integrate(MBSim::DynamicSystemSolver& system_);
      /*! Threads: Number of Threads (0,1,2 or 3) 0: auto (number of threads depends on order and SSC)*/ 
      void integrate(MBSim::DynamicSystemSolver& systemT1_, MBSim::DynamicSystemSolver& systemT2_, MBSim::DynamicSystemSolver& systemT3_, int Threads=0);

      void setAbsoluteTolerance(const fmatvec::Vec &aTol_) {aTol = aTol_;}
      void setAbsoluteTolerance(double aTol_) {aTol = fmatvec::Vec(1,fmatvec::INIT,aTol_);}
      void setRelativeTolerance(const fmatvec::Vec &rTol_) {rTol = rTol_;}
      void setRelativeTolerance(double rTol_) {rTol = fmatvec::Vec(1,fmatvec::INIT,rTol_);}
      void setgapTolerance(double gTol) {gapTol = gTol;}

      /** subroutines for integrate function */

      void preIntegrate(MBSim::DynamicSystemSolver& system);
      void subIntegrate(MBSim::DynamicSystemSolver& system, double tStop);
      void postIntegrate(MBSim::DynamicSystemSolver& system);
      void preIntegrate(MBSim::DynamicSystemSolver& systemT1_, MBSim::DynamicSystemSolver& systemT2_, MBSim::DynamicSystemSolver& systemT3_);
       
      /** internal subroutines */
      void getAllSetValuedla(fmatvec::Vec& la_,fmatvec::VecInt& la_Sizes,std::vector<MBSim::Link*> &SetValuedLinkList);
      void setAllSetValuedla(const fmatvec::Vec& la_,const fmatvec::VecInt& la_Sizes,std::vector<MBSim::Link*> &SetValuedLinkList);
      void getDataForGapControl();
      bool testTolerances();
      bool GapControl(double qUnsafe, bool SSCTestOK); 
      bool changedLinkStatus(const fmatvec::VecInt &L1, const fmatvec::VecInt &L2, int ex);
      double calculatedtNewRel(const fmatvec::Vec &ErrorLocal, double H);
      void plot();

      virtual void initializeUsingXML(xercesc::DOMElement *element);
  };

}

#endif 
