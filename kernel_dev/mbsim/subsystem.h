/* Copyright (C) 2004-2009 MBSim Development Team
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
 */

#ifndef _SUBSYSTEM_H_
#define _SUBSYSTEM_H_

#include "mbsim/element.h"
#include "mbsim/interfaces.h"
#include "mbsim/mbsim_event.h"
#ifdef HAVE_AMVISCPPINTERFACE
#include "amviscppinterface/group.h"
#endif

namespace H5 {
  class Group;
}

namespace MBSim {
  class Frame;
  class Contour;
  class ExtraDynamicInterface;
  class DataInterfaceBase;
  class Object;
  class Link;

  // TODO delete compatibility classes 
  class TreeRigid;
  class BodyRigid;

  /**
   * \brief subsystem as topmost hierarchical level
   * \author Martin Foerg
   * \date 2009-03-26 some comments (Thorsten Schindler)
   */
  class Subsystem : public Element, public ObjectInterface, public LinkInterface {
    public:
      /** 
       * \brief constructor
       */
      Subsystem(const string &name);

      /**
       * \brief destructor
       */
      virtual ~Subsystem();

      /* INHERITED INTERFACE OF OBJECTINTERFACE */
      virtual void updateT(double t); 
      virtual void updateh(double t); 
      virtual void updateM(double t); 
      virtual void updateJacobians(double t) = 0; 
      virtual void updatedq(double t, double dt); 
      virtual void updatedu(double t, double dt) = 0;
      virtual void updateud(double t) { throw new MBSimError("ERROR (Subsystem::updateud): Not implemented!"); }
      virtual void updateqd(double t) { throw new MBSimError("ERROR (Subsystem::updateud): Not implemented!"); }
      virtual void sethSize(int hSize_, int i=0);
      virtual int gethSize(int i=0) const { return hSize[i]; }
      virtual int getqSize() const { return qSize; }
      virtual int getuSize(int i=0) const { return uSize[i]; }
      virtual void calcqSize();
      virtual void calcuSize(int j=0);
      virtual void setqInd(int qInd_) { qInd = qInd_; }
      virtual void setuInd(int uInd_, int i=0) { uInd[i] = uInd_; }
      virtual int gethInd(Subsystem* sys, int i=0); 
      virtual void updateSecondJacobians(double t) = 0; 
      virtual H5::Group *getPlotGroup() { return plotGroup; }
      virtual PlotFeatureStatus getPlotFeature(PlotFeature fp) { return Element::getPlotFeature(fp); };
      virtual PlotFeatureStatus getPlotFeatureForChildren(PlotFeature fp) { return Element::getPlotFeatureForChildren(fp); };
#ifdef HAVE_AMVISCPPINTERFACE
      virtual AMVis::Group* getAMVisGrp() { return amvisGrp; }
#endif
      /*****************************************************/

      /* INHERITED INTERFACE OF LINKINTERFACE */
      virtual void updater(double t); 
      virtual void updatewb(double t); 
      virtual void updateW(double t); 
      virtual void updateV(double t); 
      virtual void updateg(double t);
      virtual void updategd(double t);
      virtual void updatedx(double t, double dt); 
      virtual void updatexd(double t);
      virtual void updateStopVector(double t); 
      /*****************************************************/

      /* INHERITED INTERFACE OF ELEMENT */
      virtual void setFullName(const string &str);
      virtual string getType() const { return "Subsystem"; }
      virtual void setMultiBodySystem(MultiBodySystem* sys);
      virtual void plot(double t, double dt);
      virtual void closePlot();
      /*****************************************************/

      /* INTERFACE FOR DERIVED CLASSES */
      /**
       * \brief initialisiation of all ingredients
       */
      virtual void init();

      /**
       * \brief do things before initialisation TODO necessary?
       */
      virtual void preinit();

      /**
       * \brief compute Cholesky decomposition of mass matrix TODO necessary?
       */
      virtual void facLLM() = 0;

      /**
       * \brief solve contact equations with single step fixed point scheme on acceleration level 
       */
      virtual int solveConstraintsFixpointSingle();

      /**
       * \brief solve contact equations with single step fixed point scheme on velocity level 
       */
      virtual int solveImpactsFixpointSingle();

      /**
       * \brief solve contact equations with Gauss-Seidel scheme on acceleration level 
       */
      virtual int solveConstraintsGaussSeidel();

      /**
       * \brief solve contact equations with Gauss-Seidel scheme on velocity level 
       */
      virtual int solveImpactsGaussSeidel();

      /**
       * \brief solve contact equations with Newton scheme on acceleration level 
       */
      virtual int solveConstraintsRootFinding();

      /**
       * \brief solve contact equations with Newton scheme on velocity level 
       */
      virtual int solveImpactsRootFinding();

      /**
       * \brief compute JACOBIAN of contact equations on acceleration level 
       */
      virtual int jacobianConstraints();

      /**
       * \brief compute JACOBIAN of contact equations on velocity level
       */
      virtual int jacobianImpacts();

      /**
       * \brief validate force laws concerning given tolerances on acceleration level
       */
      virtual void checkConstraintsForTermination();

      /**
       * \brief validate force laws concerning given tolerances on velocity level
       */
      virtual void checkImpactsForTermination();

      /**
       * \brief update relaxation factors for contact equations
       */
      virtual void updaterFactors();

      /**
       * \brief plots time series headers and manages HDF5 files of subsystems
       */
      virtual void initPlot();

      /**
       * \param name of the frame
       * \param check for existence of frame
       * \return frame
       */
      virtual Frame* getFrame(const string &name, bool check=true);

      /**
       * \param name of the contour
       * \param check for existence of contour
       * \return contour
       */
      virtual Contour* getContour(const string &name, bool check=true);
      /*****************************************************/

      /* GETTER / SETTER */
      Subsystem* getParent() { return parent; }
      void setParent(Subsystem* sys) { parent = sys; }

      const Vec& getq() const { return q; };
      const Vec& getu() const { return u; };
      const Vec& getx() const { return x; };
      Vec& getx() { return x; };
      const Vec& getxd() const { return xd; };
      Vec& getxd() { return xd; };
      const Vec& getx0() const { return x0; };
      Vec& getx0() { return x0; };

      const Mat& getT() const { return T; };
      const SymMat& getM() const { return M; };
      const SymMat& getLLM() const { return LLM; };
      const Vec& geth() const { return h; };
      const Vec& getf() const { return f; };
      Vec& getf() { return f; };

      const Mat& getW() const { return W; }
      Mat& getW() { return W; }
      const Mat& getV() const { return V; }
      Mat& getV() { return V; }

      const Vec& getla() const { return la; }
      Vec& getla() { return la; }
      const Vec& getg() const { return g; }
      Vec& getg() { return g; }
      const Vec& getgd() const { return gd; }
      Vec& getgd() { return gd; }
      const Vec& getrFactor() const { return rFactor; }
      Vec& getrFactor() { return rFactor; }
      Vec& getsv() { return sv; }
      const Vec& getsv() const { return sv; }
      Vector<int>& getjsv() { return jsv; }
      const Vector<int>& getjsv() const { return jsv; }
      const Vec& getres() const { return res; }
      Vec& getres() { return res; }

      void setx(const Vec& x_) { x = x_; }
      void setx0(const Vec &x0_) { x0 = x0_; }
      void setx0(double x0_) { x0 = Vec(1,INIT,x0_); }

      int getxInd() { return xInd; }
      int getlaInd() const { return laInd; } 

      int getqInd() { return qInd; }
      int getuInd(int i=0) { return uInd[i]; }
      int gethInd(int i=0) { return hInd[i]; }
      void setxInd(int xInd_) { xInd = xInd_; }
      void sethInd(int hInd_, int i=0) { hInd[i] = hInd_; }
      void setlaInd(int ind) { laInd = ind; }
      void setgInd(int ind) { gInd = ind; }
      void setgdInd(int ind) { gdInd = ind; }
      void setrFactorInd(int ind) { rFactorInd = ind; }
      void setsvInd(int svInd_) { svInd = svInd_; };

      int getxSize() const { return xSize; }
      int getzSize() const { return qSize + uSize[0] + xSize; }

      void setqSize(int qSize_) { qSize = qSize_; }
      void setuSize(int uSize_, int i=0) { uSize[i] = uSize_; }
      void setxSize(int xSize_) { xSize = xSize_; }

      int getlaSize() const { return laSize; } 
      int getgSize() const { return gSize; } 
      int getgdSize() const { return gdSize; } 
      int getrFactorSize() const { return rFactorSize; } 
      int getsvSize() const { return svSize; }
      /*****************************************************/

      /**
       * \brief references to positions of subsystem parent
       * \param vector to be referenced
       */
      void updateqRef(const Vec &ref); 
      
      /**
       * \brief references to differentiated positions of subsystem parent
       * \param vector to be referenced
       */
      void updateqdRef(const Vec &ref);

      /**
       * \brief references to velocities of subsystem parent
       * \param vector to be referenced
       */
      void updateuRef(const Vec &ref);

      /**
       * \brief references to differentiated velocities of subsystem parent
       * \param vector to be referenced
       */
      void updateudRef(const Vec &ref);

      /**
       * \brief references to order one parameters of subsystem parent
       * \param vector to be referenced
       */
      void updatexRef(const Vec &ref);

      /**
       * \brief references to differentiated order one parameters of subsystem parent
       * \param vector to be referenced
       */
      void updatexdRef(const Vec &ref);

      /**
       * \brief references to smooth right hand side of subsystem parent
       * \param vector to be referenced
       * \param index of normal usage and inverse kinetics
       */
      void updatehRef(const Vec &ref, int i=0);
      
      /**
       * \brief references to order one right hand side of subsystem parent
       * \param vector to be referenced
       */
      void updatefRef(const Vec &ref);
      
      /**
       * \brief references to nonsmooth right hand side of subsystem parent
       * \param vector to be referenced
       */
      void updaterRef(const Vec &ref);

      /**
       * \brief references to linear transformation matrix between differentiated positions and velocities of subsystem parent
       * \param matrix to be referenced
       */
      void updateTRef(const Mat &ref);

      /**
       * \brief references to mass matrix of subsystem parent
       * \param matrix to be referenced
       * \param index of normal usage and inverse kinetics
       */
      void updateMRef(const SymMat &ref, int i=0);

      /**
       * \brief references to Cholesky decomposition of subsystem parent
       * \param matrix to be referenced
       * \param index of normal usage and inverse kinetics
       */
      void updateLLMRef(const SymMat &ref, int i=0);

      /**
       * \brief references to relative distances of subsystem parent
       * \param vector to be referenced
       */
      void updategRef(const Vec &ref);
      
      /**
       * \brief references to relative velocities of subsystem parent
       * \param vector to be referenced
       */
      void updategdRef(const Vec &ref);
      
      /**
       * \brief references to contact force parameters of subsystem parent
       * \param vector to be referenced
       */
      void updatelaRef(const Vec &ref);

      /**
       * \brief references to TODO of subsystem parent
       * \param vector to be referenced
       */      
      void updatewbRef(const Vec &ref);

      /**
       * \brief references to contact force direction matrix of subsystem parent
       * \param matrix to be referenced
       * \param index of normal usage and inverse kinetics
       */
      void updateWRef(const Mat &ref, int i=0);

      /**
       * \brief references to condensed contact force direction matrix of subsystem parent
       * \param matrix to be referenced
       * \param index of normal usage and inverse kinetics
       */
      void updateVRef(const Mat &ref, int i=0);

      /**
       * \brief references to stopvector (rootfunction for event driven integrator) of subsystem parent
       * \param vector to be referenced
       */
      void updatesvRef(const Vec& ref);

      /**
       * \brief references to boolean evaluation of stopvector concerning roots of subsystem parent
       * \param vector to be referenced
       */
      void updatejsvRef(const Vector<int> &ref);

      /**
       * \brief references to residuum of contact equations of subsystem parent
       * \param vector to be referenced
       */
      void updateresRef(const Vec &ref);

      /**
       * \brief references to relaxation factors for contact equations of subsystem parent
       * \param vector to be referenced
       */
      void updaterFactorRef(const Vec &ref);

      /**
       * \brief initialises state variables
       */
      void initz();

      /**
       * \brief set possible attribute for active relative kinematics for updating event driven simulation before case study
       */
      void updateCondition();

      /**
       * \brief change JACOBIAN of contact size concerning use of inverse kinetics
       * \param index of normal usage and inverse kinetics
       */
      void resizeJacobians(int j);

      /**
       * \brief analyse constraints of subsystems for usage in inverse kinetics
       */
      void checkForConstraints();

      /**
       * \brief distribute links to set- and single valued container
       */
      void setUpLinks();

      /**
       * \return flag, if vector of active relative distances has changed
       */
      bool gActiveChanged();

      /**
       * \brief calculates size of order one parameters
       */
      void calcxSize();
      
      /**
       * \brief calculates size of stop vector
       */
      void calcsvSize();

      /**
       * \brief calculates size of contact force parameters
       */
      void calclaSize();

      /**
       * \brief calculates size of active contact force parameters
       */
      void calclaSizeForActiveg();

      /**
       * \brief calculates size of relative distances
       */
      void calcgSize();

      /**
       * \brief calculates size of active relative distances
       */
      void calcgSizeActive();

      /**
       * \brief calculates size of relative velocities
       */
      void calcgdSize();

      /**
       * \brief calculates size of active relative velocities
       */
      void calcgdSizeActive();

      /**
       * \brief calculates size of relaxation factors for contact equations
       */
      void calcrFactorSize();

      /** 
       * \brief rearrange vector of active setvalued links
       */
      void checkActiveLinks();

      /**
       * \brief set possible attribute for active relative distance in derived classes 
       */
      void checkActiveg();

      /**
       * \brief set possible attribute for active relative velocity in derived classes for initialising event driven simulation 
       */
      void checkActivegd();

      /**
       * \brief set possible attribute for active relative velocity in derived classes for updating event driven simulation after an impact
       */
      void checkActivegdn();

      /**
       * \brief set possible attribute for active relative acceleration in derived classes for updating event driven simulation after an impact
       */
      void checkActivegdd();

      /**
       * \brief set possible attribute for active relative velocity in derived classes for updating event driven and time-stepping simulation before an impact
       */
      void checkAllgd();

      /** 
       * \param scale factor for flow quantity tolerances tolQ=tol*scaleTolQ TODO necessary?
       */
      void setScaleTolQ(double scaleTolQ);

      /** 
       * \param scale factor for pressure quantity tolerances tolp=tol*scaleTolp TODO necessary?
       */
      void setScaleTolp(double scaleTolp);

      /**
       * \param tolerance for relative velocity
       */
      void setgdTol(double tol);
      
      /**
       * \param tolerance for relative acceleration
       */
      void setgddTol(double tol);

      /**
       * \param tolerance for contact force
       */
      void setlaTol(double tol);

      /**
       * \param tolerance for impact
       */
      void setLaTol(double tol);

      /**
       * \param maximum relaxation factor for contact equations
       */
      void setrMax(double rMax);

      /**
       * \brief TODO
       */
      void setlaIndMBS(int laIndParent);

      /**
       * \param frame to add
       */
      void addFrame(Frame *frame_);

      /**
       * \param frame to add
       * \param relative position of frame
       * \param relative orientation of frame
       * \param relation frame
       */
      void addFrame(Frame *frame_, const Vec &RrRK, const SqrMat &ARK, const Frame* refFrame=0);

      /**
       * \param name of frame to add
       * \param relative position of frame
       * \param relative orientation of frame
       * \param relation frame
       */
      void addFrame(const string &str, const Vec &RrRK, const SqrMat &ARK, const Frame* refFrame=0);

      /**
       * \param contour to add
       */
      void addContour(Contour* contour);

      /**
       * \param contour to add
       * \param relative position of contour
       * \param relative orientation of contour
       * \param relation frame
       */
      void addContour(Contour* contour, const Vec &RrRC, const SqrMat &ARC, const Frame* refFrame=0);

      /**
       * \param contour to add
       * \param relative position of contour
       * \param relation frame
       */
      void addContour(Contour* contour, const Vec &RrRC, const Frame* refFrame=0) { addContour(contour,RrRC,SqrMat(3,EYE)); }

      /**
       * \param frame
       * \return index of frame TODO renaming
       */
      int portIndex(const Frame *frame_) const;

      /**
       * \param subsystem to add
       */
      void addSubsystem(Subsystem *subsystem);

      /**
       * \param name of the subsystem
       * \param check for existence of subsystem
       * \return subsystem
       */
      Subsystem* getSubsystem(const string &name,bool check=true);
      
      /**
       * \param object to add
       */
      void addObject(Object *object);
      
      /**
       * \param name of the object
       * \param check for existence of object
       * \return object
       */
      Object* getObject(const string &name,bool check=true);

      /**
       * \param link to add
       */
      void addLink(Link *link);

      /**
       * \param name of the link
       * \param check for existence of link
       * \return link
       */
      Link* getLink(const string &name,bool check=true);
      
      /**
       * \param extra dynamic interface to add
       */
      void addEDI(ExtraDynamicInterface *edi_);
      
      /**
       * \param name of the extra dynamic interface
       * \param check for existence of extra dynamic interface
       * \return extra dynamic interface
       */
      ExtraDynamicInterface* getEDI(const string &name,bool check=true);

      /**
       * \param data interface base to add
       */
      void addDataInterfaceBase(DataInterfaceBase* dib_);
      
      /**
       * \param name of the data interface interface
       * \param check for existence of data interface interface
       * \return data interface interface
       */
      DataInterfaceBase* getDataInterfaceBase(const string &name, bool check=true);

    protected:
      /**
       * \brief parent subsystem
       */
      Subsystem *parent;

      /** 
       * \brief container for possible ingredients
       */
      vector<Object*> object;
      vector<Link*> link;
      vector<ExtraDynamicInterface*> EDI;
      vector<DataInterfaceBase*> DIB;
      vector<Subsystem*> subsystem;
      vector<Link*> linkSingleValued;
      vector<Link*> linkSetValued;
      vector<Link*> linkSetValuedActive;

      /** 
       * \brief linear relation matrix of position and velocity parameters
       */
      Mat T;

      /**
       * \brief mass matrix
       */
      SymMat M;

      /** 
       * \brief Cholesky decomposition of mass matrix
       */
      SymMat LLM;

      /**
       * \brief positions, differentiated positions, initial positions
       */
      Vec q, qd, q0;

      /**
       * \brief velocities, differentiated velocities, initial velocities
       */
      Vec u, ud, u0;

      /**
       * \brief order one parameters, differentiated order one parameters, initial order one parameters
       */
      Vec x, xd, x0;

      /**
       * \brief smooth, nonsmooth and order one right hand side
       */
      Vec h, r, f;

      /**
       * \brief 
       */
      Mat W, V;

      /**
       * \brief contact force parameters
       */
      Vec la;

      /** 
       * \brief relative distances and velocities
       */
      Vec g, gd;

      /**
       * \brief TODO
       */
      Vec wb;

      /**
       * \brief residuum of nonlinear contact equations for Newton scheme
       */
      Vec res;

      /**
       * \brief rfactors for relaxation nonlinear contact equations
       */
      Vec rFactor;

      /**
       * \brief stop vector (root functions for event driven integration
       */
      Vec sv;

      /**
       * \brief boolean evaluation of stop vector concerning roots
       */
      Vector<int> jsv;

      /** 
       * \brief size and local start index of positions
       */
      int qSize, qInd;

      /** 
       * \brief size and local start index of velocities
       */
      int uSize[2], uInd[2];

      /** 
       * \brief size and local start index of order one parameters
       */
      int xSize, xInd;

      /** 
       * \brief size and local start index of order smooth right hand side
       */
      int hSize[2], hInd[2];

      /** 
       * \brief size and local start index of relative distances
       */
      int gSize, gInd;

      /** 
       * \brief size and local start index of relative velocities
       */
      int gdSize, gdInd;

      /** 
       * \brief size and local start index of contact force parameters
       */
      int laSize, laInd;

      /** 
       * \brief size and local start index of rfactors
       */
      int rFactorSize, rFactorInd;

      /** 
       * \brief size and local start index of stop vector
       */
      int svSize, svInd;

      /**
       * \brief inertial position of frames, contours and TODO
       */
      vector<Vec> IrOK, IrOC, IrOS;

      /**
       * \brief orientation to inertial frame of frames, contours and TODO
       */
      vector<SqrMat> AIK, AIC, AIS;

      /**
       * \brief vector of frames and contours
       */
      vector<Frame*> port;
      vector<Contour*> contour;

#ifdef HAVE_AMVISCPPINTERFACE
      AMVis::Group* amvisGrp;
#endif
  };
}

#endif

