/* Copyright (C) 2004-2011 MBSim Development Team
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
 * Contact: thorsten.schindler@mytum.de
 */

#ifndef _FLEXIBLE_BODY_1S_33_COSSERAT_H_
#define _FLEXIBLE_BODY_1S_33_COSSERAT_H_

#include "mbsimFlexibleBody/flexible_body.h"
#include "mbsimFlexibleBody/pointer.h"
#include "mbsimFlexibleBody/contours/flexible_band.h"
#include "mbsimFlexibleBody/contours/cylinder_flexible.h"
#include "mbsimFlexibleBody/flexible_body/finite_elements/finite_element_1s_33_cosserat_translation.h"
#include "mbsimFlexibleBody/flexible_body/finite_elements/finite_element_1s_33_cosserat_rotation.h"
#ifdef HAVE_OPENMBVCPPINTERFACE
#include <openmbvcppinterface/spineextrusion.h>
#endif

namespace MBSimFlexibleBody {

  class NurbsCurve1s;

  /**
   * \brief finite element for spatial beam using Cosserat model
   * \author Thorsten Schindler
   * \author Christian Käsbauer
   * \author Thomas Cebulla
   * \date 2011-09-10 initial commit (Thorsten Schindler)
   * \data 2011-10-08 basics derived and included (Thorsten Schindler)
   * \date 2011-10-12 rotation grid added (Thorsten Schindler)
   * \todo compute boundary conditions TODO
   *
   * Cosserat model based on
   * H. Lang, J. Linn, M. Arnold: Multi-body dynamics simulation of geometrically exact Cosserat rods
   * but with 
   *  - Kirchhoff assumption (-> less stiff)
   *  - Cardan parametrisation (-> less problems with condition and drift for quaternion dae system)
   *  - piecewise constant Darboux vector with evaluation according to
   *    I. Romero: The interpolation of rotations and its application to finite element models of
   *    geometrically exact beams
   */
  class FlexibleBody1s33Cosserat : public FlexibleBodyContinuum<double> {
    public:

      /**
       * \brief constructor
       * \param name of body
       * \param bool to specify open (cantilever) or closed (ring) structure
       */
      FlexibleBody1s33Cosserat(const std::string &name, bool openStructure); 

      /**
       * \brief destructor
       */
      virtual ~FlexibleBody1s33Cosserat();

      /* INHERITED INTERFACE OF FLEXIBLE BODY */
      virtual void BuildElements();
      virtual void GlobalVectorContribution(int n, const fmatvec::Vec& locVec, fmatvec::Vec& gloVec);
      virtual void GlobalMatrixContribution(int n, const fmatvec::Mat& locMat, fmatvec::Mat& gloMat);
      virtual void GlobalMatrixContribution(int n, const fmatvec::SymMat& locMat, fmatvec::SymMat& gloMat);
      virtual void updateKinematicsForFrame(MBSim::ContourPointData &cp, MBSim::FrameFeature ff, MBSim::Frame *frame=0);
      virtual void updateJacobiansForFrame(MBSim::ContourPointData &data, MBSim::Frame *frame=0);
      /***************************************************/

      /* INHERITED INTERFACE OF OBJECT */
      virtual void init(MBSim::InitStage stage);
      virtual double computePotentialEnergy();
      virtual void facLLM(int i=0);
      /***************************************************/

      /* INHERITED INTERFACE OF OBJECTINTERFACE */
      virtual void updateh(double t, int i=0);
      virtual void updateStateDependentVariables(double t);

      /* INHERITED INTERFACE OF ELEMENT */
      virtual void plot(double t, double dt=1);
      virtual std::string getType() const { return "FlexibleBody1s33Cosserat"; }
      /***************************************************/

      /* GETTER / SETTER */
      void setNumberElements(int n);   	
      void setLength(double L_);   	
      void setEGModuls(double E_,double G_);    	
      void setDensity(double rho_);	
      void setCrossSectionalArea(double A_);    	
      void setMomentsInertia(double I1_,double I2_,double I0_);
      void setCurlRadius(double R1_,double R2_);
      void setMaterialDamping(double cEps0D_,double cEps1D_,double cEps2D_);
      void setCylinder(double cylinderRadius_);
      void setCuboid(double cuboidBreadth_,double cuboidHeight_);

#ifdef HAVE_OPENMBVCPPINTERFACE
      void setOpenMBVSpineExtrusion(OpenMBV::SpineExtrusion* body) { openMBVBody=body; }
#endif

      int getNumberElements() const { return Elements; }   	
      double getLength() const { return L; }
      bool isOpenStructure() const { return openStructure; }
      /***************************************************/

      /**
       * \brief compute state (positions, angles, velocities, differentiated angles) at Lagrangian coordinate in local FE coordinates
       * \param Lagrangian coordinate
       */
      fmatvec::Vec computeState(double s);

    private:
      /** 
       * \brief stl-vector of finite elements for rotation grid
       */
      std::vector<MBSim::DiscretizationInterface*> rotationDiscretization;

      /** 
       * \brief stl-vector of finite element positions for rotation grid
       */
      std::vector<fmatvec::Vec> qRotationElement;

      /** 
       * \brief stl-vector of finite element wise velocities for rotation grid
       */
      std::vector<fmatvec::Vec> uRotationElement;

      /**
       * \brief contours
       */
      CylinderFlexible *cylinder;
      FlexibleBand *top, *bottom, *left, *right;

      /**
       * \brief angle parametrisation
       */
      CardanPtr angle;

      /**
       * \brief number of elements
       */
      int Elements;

      /**
       * \brief length of entire beam and finite elements
       */
      double L, l0;

      /**
       * \brief elastic modules
       */
      double E, G;

      /**
       * \brief area of cross-section
       */
      double A;

      /**
       * \brief area moment of inertia 
       */
      double I1, I2, I0;

      /**
       * \brief density 
       */
      double rho;

      /**
       * \brief radius of undeformed shape
       */
      double R1, R2;

      /**
       * \brief strain damping 
       */
      double cEps0D, cEps1D, cEps2D;

      /**
       * \brief open or closed beam structure
       */
      bool openStructure;

      /**
       * \brief initialised FLAG 
       */
      bool initialised;

      /**
       * \brief boundary conditions for rotation grid
       * first and last finite difference rotation beam element refer to values not directly given by dof in open structure
       * they have to be estimated by the following values calculated in computeBoundaryCondition() 
       */
      fmatvec::Vec bound_ang_start;
      fmatvec::Vec bound_ang_end;
      fmatvec::Vec bound_ang_vel_start;
      fmatvec::Vec bound_ang_vel_end;

      /**
       * \brief contour data 
       */
      double cuboidBreadth, cuboidHeight, cylinderRadius;

      /**
       * \brief contour for state description
       */
      NurbsCurve1s *curve;

      FlexibleBody1s33Cosserat(); // standard constructor
      FlexibleBody1s33Cosserat(const FlexibleBody1s33Cosserat&); // copy constructor
      FlexibleBody1s33Cosserat& operator=(const FlexibleBody1s33Cosserat&); // assignment operator

      /**
       * \brief detect current finite element
       * \param global parametrisation
       * \param local parametrisation
       * \param finite element number
       */
      void BuildElement(const double& sGlobal, double& sLocal, int& currentElement);
      
      /**
       * \brief initialize translational part of mass matrix and calculate Cholesky decomposition
       */
      void initM();
      
      /**
       * \brief compute boundary conditions for rotation grid
       * first and last finite difference rotation beam element refer to values not directly given by dof in open structure
       * they have to be estimated in the following function
       */
      void computeBoundaryCondition();
      
      /** 
       * \brief insert 'local' information in global vectors for rotation grid
       * \param number of finite element
       * \param local vector
       * \param global vector
       */
      void GlobalVectorContributionRotation(int n, const fmatvec::Vec& locVec, fmatvec::Vec& gloVec);
  };

  inline void FlexibleBody1s33Cosserat::setLength(double L_) { L = L_; }
  inline void FlexibleBody1s33Cosserat::setEGModuls(double E_,double G_) { E = E_; G = G_; }    	
  inline void FlexibleBody1s33Cosserat::setDensity(double rho_) { rho = rho_;}     	
  inline void FlexibleBody1s33Cosserat::setCrossSectionalArea(double A_) { A = A_; }    	
  inline void FlexibleBody1s33Cosserat::setMomentsInertia(double I1_, double I2_, double I0_) { I1 = I1_; I2 = I2_; I0 = I0_; }    	
  inline void FlexibleBody1s33Cosserat::setCurlRadius(double R1_,double R2_) { R1 = R1_; R2 = R2_; if(initialised) for(int i=0;i<Elements;i++) static_cast<FiniteElement1s33CosseratRotation*>(rotationDiscretization[i])->setCurlRadius(R1,R2); }    	
  inline void FlexibleBody1s33Cosserat::setMaterialDamping(double cEps0D_,double cEps1D_,double cEps2D_) { cEps0D = cEps0D_; cEps1D = cEps1D_; cEps2D = cEps2D_; if(initialised) for(int i=0;i<Elements;i++) static_cast<FiniteElement1s33CosseratTranslation*>(discretization[i])->setMaterialDamping(Elements*cEps0D,cEps1D,cEps2D); }
  inline void FlexibleBody1s33Cosserat::setCylinder(double cylinderRadius_) { cylinderRadius = cylinderRadius_; }
  inline void FlexibleBody1s33Cosserat::setCuboid(double cuboidBreadth_,double cuboidHeight_) { cuboidBreadth = cuboidBreadth_; cuboidHeight = cuboidHeight_; }

}

#endif /* _FLEXIBLE_BODY_1S_33_COSSERAT_H_ */
