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

#ifndef FUNCTIONS_CONTACT_H_
#define FUNCTIONS_CONTACT_H_

#include <mbsim/contour.h>
#include <mbsim/contours/contour1s.h>
#include <mbsim/utils/function.h>

namespace MBSim {

  /*! 
   * \brief class for distances and root functions of contact problems
   * \author Roland Zander
   * \date 2009-04-21 some comments (Thorsten Schindler)
   */
  template<class Ret, class Arg>
    class DistanceFunction : public Function<Ret,Arg> {
      public:
        /* INTERFACE FOR DERIVED CLASSES */
        /*!
         * \param contour parameter
         * \return root function evaluation at contour parameter
         */
        virtual Ret operator()(const Arg& x) = 0;

        /*!
         * \param contour parameter
         * \return possible contact-distance at contour parameter
         */
        virtual double operator[](const Arg& x) { return nrm2(computeWrD(x)); };	

        /*!
         * \param contour parameter
         * \return helping distance vector at contour parameter
         */
        virtual fmatvec::Vec computeWrD(const Arg& x) = 0;
        /*************************************************/
    };

  /*!
   * \brief root function for pairing Contour1s and Point
   * \author Roland Zander
   * \date 2009-04-21 contour point data included (Thorsten Schindler)
   */
  class FuncPairContour1sPoint : public DistanceFunction<double,double> {
    public:
      /**
       * \brief constructor
       * \param point contour
       * \param contour with one contour parameter
       */
      FuncPairContour1sPoint(Point* point_, Contour1s *contour_) : contour(contour_), point(point_), cp(fmatvec::Vec(1,fmatvec::INIT,0.)) {}

      /* INHERITED INTERFACE OF DISTANCEFUNCTION */
      double operator()(const double &alpha) {
        fmatvec::Vec Wd = computeWrD(alpha);
        fmatvec::Vec Wt = cp.getFrameOfReference().getOrientation().col(1);
        return trans(Wt)*Wd;
      }

      fmatvec::Vec computeWrD(const double &alpha) {
        if(alpha!=cp.getLagrangeParameterPosition()(0)) {
          cp.getLagrangeParameterPosition()(0) = alpha;
          contour->computeRootFunctionPosition(cp);
          contour->computeRootFunctionFirstTangent(cp);
        }
        return point->getFrame()->getPosition() - cp.getFrameOfReference().getPosition();
      }
      /*************************************************/

    private:
      /**
       * \brief contours
       */
      Contour1s *contour;
      Point *point;

      /**
       * \brief contour point data for saving old values
       */
      ContourPointData cp;
  };

  /*!
   * \brief root function for pairing CylinderFlexible and CircleHollow
   * \author Roland Zander
   * \date 2009-04-21 contour point data included (Thorsten Schindler)
   */
  class FuncPairContour1sCircleHollow : public DistanceFunction<double,double> {
    public:
      /**
       * \brief constructor
       * \param circle hollow contour
       * \param contour with one contour parameter
       */
      FuncPairContour1sCircleHollow(CircleHollow* circle_, Contour1s *contour_) : contour(contour_), circle(circle_) {}
      
      /* INHERITED INTERFACE OF DISTANCEFUNCTION */
      double operator()(const double &alpha) {
        fmatvec::Vec Wd = computeWrD(alpha);
        return trans(circle->getReferenceOrientation().col(2))*Wd;
      }

      fmatvec::Vec computeWrD(const double &alpha) {
        if(alpha!=cp.getLagrangeParameterPosition()(0)) {
          cp.getLagrangeParameterPosition()(0) = alpha;
          contour->computeRootFunctionPosition(cp);
        }
        return circle->getFrame()->getPosition() - cp.getFrameOfReference().getPosition();
      }
      /*************************************************/
    
    private:
      /**
       * \brief contours
       */
      Contour1s *contour;      
      CircleHollow *circle;
      
      /**
       * \brief contour point data for saving old values
       */
      ContourPointData cp;
  };


  /*! Root function for pairing Contour1s and Point */
  class FuncPairPointContourInterpolation : public DistanceFunction<fmatvec::Vec,fmatvec::Vec> {
    private:
      ContourInterpolation *contour;
      Point *point;
    public:
      FuncPairPointContourInterpolation(Point* point_, ContourInterpolation *contour_) : contour(contour_), point(point_) {}
      fmatvec::Vec operator()(const fmatvec::Vec &alpha) {
        fmatvec::Mat Wt = contour->computeWt(alpha);
        fmatvec::Vec WrOC[2];
        WrOC[0] = point->getFrame()->getPosition();
        WrOC[1] = contour->computeWrOC(alpha);
        //      fmatvec::Vec Wd = WrOC[1] - WrOC[0];
        return trans(Wt) * ( WrOC[1] - WrOC[0] ); //Wd;
      }
      fmatvec::Vec computeWrD(const fmatvec::Vec &alpha) {
        return contour->computeWrOC(alpha) - point->getFrame()->getPosition();
      }
  };

  /*! \brief Base Root function for planar pairing Cone-Section and Circle 
   * 
   * Author:  Thorsten Schindler
   */
  class FuncPairConeSectionCircle : public DistanceFunction<double,double> {
    public:
      /*! Constructor with \default el_IN_ci=true to distinguish relative position of ellipse */
      FuncPairConeSectionCircle(double R_,double a_,double b_) : R(R_), a(a_), b(b_), sec_IN_ci(true) {}
      /*! Constructor */
      FuncPairConeSectionCircle(double R_,double a_,double b_,bool sec_IN_ci_) : R(R_), a(a_), b(b_), sec_IN_ci(sec_IN_ci_) {}

      /*! Set distance vector of circle- and cone-section midpoint M_S-M_C */
      void setDiffVec(fmatvec::Vec d_);
      /*! Set the normed base-vectors of the cone-section */
      void setSectionCOS(fmatvec::Vec b1_,fmatvec::Vec b2_);

      /*! Return value of the root-function at cone-section-parameter */
      virtual double operator()(const double &phi) = 0;
      /*! Return distance-vector of cone-section possible contact point and circle midpoint at cone-section-parameter */
      virtual fmatvec::Vec computeWrD(const double &phi) = 0;
      /*! Return distance of cone-section- and circle possible contact points at cone-section-parameter */ 
      double operator[](const double &phi);

    protected:
      /** radius of circle as well as length in b1- and b2-dirction */
      double R, a, b;

      /** cone-section in circle */
      bool sec_IN_ci;

      /** normed base-vectors of cone-section */
      fmatvec::Vec b1, b2;

      /** distance-vector of circle- and cone-section-midpoint */
      fmatvec::Vec d;  
  };

  inline void FuncPairConeSectionCircle::setDiffVec(fmatvec::Vec d_) {d=d_;}
  inline void FuncPairConeSectionCircle::setSectionCOS(fmatvec::Vec b1_,fmatvec::Vec b2_) {b1=b1_; b2=b2_;}
  inline double FuncPairConeSectionCircle::operator[](const double &phi) {if(sec_IN_ci) return R - nrm2(computeWrD(phi)); else return nrm2(computeWrD(phi)) - R;}

  /*! \brief Root function for planar pairing Ellipse and Circle 
   * 
   * Authors: Roland Zander and Thorsten Schindler
   */
  class FuncPairEllipseCircle : public FuncPairConeSectionCircle {
    public:
      /*! Constructor with \default el_IN_ci=true to distinguish relative position of ellipse */
      FuncPairEllipseCircle(double R_,double a_,double b_) : FuncPairConeSectionCircle(R_,a_,b_) {}
      /*! Constructor */
      FuncPairEllipseCircle(double R_,double a_,double b_,bool el_IN_ci_) : FuncPairConeSectionCircle(R_,a_,b_,el_IN_ci_) {}

      /*! Set the normed base-vectors of the ellipse */
      void setEllipseCOS(fmatvec::Vec b1e_,fmatvec::Vec b2e_);

      /*! Return value of the root-function at ellipse-parameter */
      double operator()(const double &phi);
      /*! Return distance-vector of ellipse possible contact point and circle midpoint at ellipse-parameter */
      fmatvec::Vec computeWrD(const double &phi);
  };

  inline void FuncPairEllipseCircle::setEllipseCOS(fmatvec::Vec b1e_,fmatvec::Vec b2e_) {setSectionCOS(b1e_,b2e_);}
  inline double FuncPairEllipseCircle::operator()(const double &phi) {return -2*b*(b2(0)*d(0) + b2(1)*d(1) + b2(2)*d(2))*cos(phi) + 2*a*(b1(0)*d(0) + b1(1)*d(1) + b1(2)*d(2))*sin(phi) + ((a*a) - (b*b))*sin(2*phi);}
  inline fmatvec::Vec FuncPairEllipseCircle::computeWrD(const double &phi) {return d + b1*a*cos(phi) + b2*b*sin(phi);}

  /*! \brief Root function for planar pairing Hyperbola and Circle 
   * 
   * Author: Bastian Esefeld
   */
  class FuncPairHyperbolaCircle : public FuncPairConeSectionCircle {

    public:
      /*! Constructor with \default hy_in_ci=true to distinguish relative position of ellipse */
      FuncPairHyperbolaCircle(double R_, double a_, double b_) : FuncPairConeSectionCircle(R_,a_,b_) {}
      /*! Constructor */
      FuncPairHyperbolaCircle(double R_, double a_, double b_, bool hy_IN_ci_) : FuncPairConeSectionCircle(R_,a_,b_,hy_IN_ci_) {}

      /*! Return value of the root-function at hyperbola-parameter */
      double operator()(const double &phi);
      /*! Return distance-vector of hyperbola possible contact point and circle midpoint at hyperbola-parameter */
      fmatvec::Vec computeWrD(const double &phi);
  };

  inline double FuncPairHyperbolaCircle::operator()(const double &phi) { return -2*b*(b2(0)*d(0) + b2(1)*d(1) + b2(2)*d(2))*cosh(phi) - 2*a*(b1(0)*d(0) + b1(1)*d(1) + b1(2)*d(2))*sinh(phi) - ((a*a) + (b*b))*sinh(2*phi);}
  inline fmatvec::Vec FuncPairHyperbolaCircle::computeWrD(const double &phi) {return d + b1*a*cosh(phi) + b2*b*sinh(phi);}

  /// TODO: An neues Design anpassen
  //////      /*! Root function for pairing Contour1s and Line */
  //////      class FuncPairContour1sLine : public DistanceFunction<double,double> {
  //////        private:
  //////          Contour1s *contour;
  //////          Line *line;
  //////        public:
  //////          FuncPairContour1sLine(Line* line_, Contour1s *contour_) : contour(contour_), line(line_) {}
  //////          double operator()(const double &s) {
  //////    	fmatvec::Vec WtC = (contour->computeWt(s)).col(0);
  //////    	fmatvec::Vec WnL = line->computeWn();
  //////    	return trans(WtC)*WnL;
  //////          }
  //////          fmatvec::Vec computeWrD(const double &s) {
  //////    	fmatvec::Vec WrOCContour =  contour->computeWrOC(s);
  //////    	fmatvec::Vec Wn = contour->computeWn(s);
  //////    	double g =trans(Wn)*(WrOCContour-line->getFrame()->getPosition()); 
  //////    	//fmatvec::Vec WrOCLine = WrOCContour-Wn*g; 
  //////    	//return WrOCContour-WrOCLine;
  //////    	return Wn*g;
  //////          }
  //////          double operator[](const double &s) {
  //////    	return nrm2(computeWrD(s));
  //////          }
  //////      };

  /*!
   * \brief root function for pairing Contour1s and Circle
   * \author Roland Zander
   * \date 2009-04-21 contour point data included (Thorsten Schindler)
   */
  class FuncPairContour1sCircleSolid : public DistanceFunction<double,double> {
    public:
      /**
       * \brief constructor
       * \param circle solid contour
       * \param contour with one contour parameter
       */
      FuncPairContour1sCircleSolid(CircleSolid* circle_, Contour1s *contour_) : contour(contour_), circle(circle_) {}
      
      /* INHERITED INTERFACE OF DISTANCEFUNCTION */
      double operator()(const double &alpha) {
        fmatvec::Vec Wd = computeWrD(alpha);
        fmatvec::Vec Wt = cp.getFrameOfReference().getOrientation().col(1);
        return trans(Wt)*Wd;
      }

      fmatvec::Vec computeWrD(const double &alpha) {
        if(alpha!=cp.getLagrangeParameterPosition()(0)) {
          cp.getLagrangeParameterPosition()(0) = alpha;
          contour->computeRootFunctionPosition(cp);
          contour->computeRootFunctionFirstTangent(cp);
          contour->computeRootFunctionNormal(cp);
        }
        fmatvec::Vec WrOC[2];
        WrOC[0] = circle->getFrame()->getPosition() - circle->getRadius()*cp.getFrameOfReference().getOrientation().col(0);
        WrOC[1] = cp.getFrameOfReference().getPosition();
        return WrOC[1] - WrOC[0];
      }
      /***************************************************/
    
    private:
      /**
       * \brief contours
       */
      Contour1s *contour;
      CircleSolid *circle;
      
      /**
       * \brief contour point data for saving old values
       */
      ContourPointData cp;
  };

  /*! 
   * \brief General class for contact search with respect to one Contour-parameter
   * \author Roland Zander
   *
   * General remarks:
   * - both operators () and [] are necessary to calculate the root-function "()" and the distance of possible contact points "[]"
   * - then it is possible to compare different root-values during e.g. regula falsi
   */
  class Contact1sSearch {
    private:
      /** initial value for Newton method */
      double s0;

      /** nodes defining search-areas for Regula-Falsi */
      fmatvec::Vec nodes;

      /** distance-function holding all information for contact-search */
      DistanceFunction<double,double> *func;

      /** all area searching by Regular-Falsi or known initial value for Newton-Method? */
      bool searchAll;

    public:
      /*! Constructor with \default searchAll = false */
      Contact1sSearch(DistanceFunction<double,double> *func_) : s0(0.),func(func_),searchAll(false) {}
      /*! Set nodes defining search-areas for Regula-Falsi */
      void setNodes(const fmatvec::Vec &nodes_) {nodes=nodes_;}
      /*! Set equally distanced nodes beginning in \param x0 with \param n search areas and width \param dx, respectively	*/
      void setEqualSpacing(const int &n, const double &x0, const double &dx);
      /*! Force search in all search areas */
      void setSearchAll(bool searchAll_) {searchAll=searchAll_;}
      /*! Set initial value for Newton-search */
      void setInitialValue(const double &s0_ ) {s0=s0_;}
      /*! Find point with minimal distance at contour-parameter */
      double slv();
  };

}

#endif /* FUNCTIONS_CONTACT_H_ */

