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
 * Contact: martin.o.foerg@googlemail.com
 */

#ifndef _SPRINGDAMPER_H_
#define _SPRINGDAMPER_H_

#include "mbsim/link_mechanics.h"
#include <mbsim/frame.h>
#include "mbsim/functions/function.h"

#ifdef HAVE_OPENMBVCPPINTERFACE
#include "mbsim/utils/boost_parameters.h"
#include "mbsim/utils/openmbv_utils.h"
#endif

namespace MBSim {

  class RigidBody;

  /** \brief A spring damper force law.
   * This class connects two frames and applies a force in it, which depends in the
   * distance and relative velocity between the two frames.
   */
  class SpringDamper : public LinkMechanics {
    protected:
      double dist;
      fmatvec::Vec3 n;
      Function<double(double,double)> *func;
#ifdef HAVE_OPENMBVCPPINTERFACE
      boost::shared_ptr<OpenMBV::CoilSpring> coilspringOpenMBV;
#endif
    public:
      SpringDamper(const std::string &name="");
      ~SpringDamper();
      void updateh(double, int i=0);
      void updateg(double);
      void updategd(double);

      /** \brief Connect the SpringDamper to frame1 and frame2 */
      void connect(Frame *frame1, Frame* frame2);

      /*INHERITED INTERFACE OF LINK*/
      bool isActive() const { return true; }
      bool gActiveChanged() { return false; }
      bool isSingleValued() const { return true; }
      std::string getType() const { return "SpringDamper"; }
      void init(InitStage stage);
      /*****************************/

      /** \brief Set function for the force calculation.
       * The first input parameter to that function is the distance g between frame2 and frame1.
       * The second input parameter to that function is the relative velocity gd between frame2 and frame1.
       * The return value of that function is used as the force of the SpringDamper.
       */
      void setForceFunction(Function<double(double,double)> *func_) {
        func=func_;
        func->setParent(this);
        func->setName("Force");
      }

      void plot(double t, double dt=1);
      void initializeUsingXML(xercesc::DOMElement *element);

#ifdef HAVE_OPENMBVCPPINTERFACE
      /** \brief Visualise the SpringDamper using a OpenMBV::CoilSpring */
      BOOST_PARAMETER_MEMBER_FUNCTION( (void), enableOpenMBVCoilSpring, tag, (optional (numberOfCoils,(int),3)(springRadius,(double),1)(crossSectionRadius,(double),-1)(nominalLength,(double),-1)(type,(OpenMBV::CoilSpring::Type),OpenMBV::CoilSpring::tube)(diffuseColor,(const fmatvec::Vec3&),"[-1;1;1]")(transparency,(double),0))) { 
        OpenMBVCoilSpring ombv(springRadius,crossSectionRadius,1,numberOfCoils,nominalLength,type,diffuseColor,transparency);
        coilspringOpenMBV=ombv.createOpenMBV();
      }

      /** \brief Visualize a force arrow acting on each of both connected frames */
     BOOST_PARAMETER_MEMBER_FUNCTION( (void), enableOpenMBVForce, tag, (optional (scaleLength,(double),1)(scaleSize,(double),1)(referencePoint,(OpenMBV::Arrow::ReferencePoint),OpenMBV::Arrow::toPoint)(diffuseColor,(const fmatvec::Vec3&),"[-1;1;1]")(transparency,(double),0))) { 
        OpenMBVArrow ombv(diffuseColor,transparency,OpenMBV::Arrow::toHead,referencePoint,scaleLength,scaleSize);
        std::vector<bool> which; which.resize(2, true);
        LinkMechanics::setOpenMBVForceArrow(ombv.createOpenMBV(), which);
      }
#endif
    private:
      std::string saved_ref1, saved_ref2;
  };

  /** \brief A spring damper force law.
   * This class connects two frames and applies a force in it, which depends in the
   * distance and relative velocity between the two frames.
   */
  class DirectionalSpringDamper : public LinkMechanics {
    protected:
      double dist;
      Function<double(double,double)> *func;
      Frame *refFrame;
      fmatvec::Vec3 forceDir, WforceDir, WrP0P1;
      Frame C;
#ifdef HAVE_OPENMBVCPPINTERFACE
      boost::shared_ptr<OpenMBV::CoilSpring> coilspringOpenMBV;
#endif
    public:
      DirectionalSpringDamper(const std::string &name="");
      ~DirectionalSpringDamper();
      void updateh(double, int i=0);
      void updateg(double);
      void updategd(double);

      /** \brief Connect the SpringDamper to frame1 and frame2 */
      void connect(Frame *frame1, Frame* frame2);

      /*INHERITED INTERFACE OF LINK*/
      bool isActive() const { return true; }
      bool gActiveChanged() { return false; }
      bool isSingleValued() const { return true; }
      std::string getType() const { return "DirectionalSpringDamper"; }
      void init(InitStage stage);
      /*****************************/

      /** \brief Set function for the force calculation.
       * The first input parameter to that function is the distance g between frame2 and frame1.
       * The second input parameter to that function is the relative velocity gd between frame2 and frame1.
       * The return value of that function is used as the force of the SpringDamper.
       */
      void setForceFunction(Function<double(double,double)> *func_) {
        func=func_;
        func->setParent(this);
        func->setName("Force");
      }

      /**
       * \param local force direction represented in first frame
       */
      void setForceDirection(const fmatvec::Vec3 &dir) { forceDir=dir/nrm2(dir); }

      void plot(double t, double dt=1);
      void initializeUsingXML(xercesc::DOMElement *element);

#ifdef HAVE_OPENMBVCPPINTERFACE
      BOOST_PARAMETER_MEMBER_FUNCTION( (void), enableOpenMBVCoilSpring, tag, (optional (numberOfCoils,(int),3)(springRadius,(double),1)(crossSectionRadius,(double),-1)(nominalLength,(double),-1)(type,(OpenMBV::CoilSpring::Type),OpenMBV::CoilSpring::tube)(diffuseColor,(const fmatvec::Vec3&),"[-1;1;1]")(transparency,(double),0))) { 
        OpenMBVCoilSpring ombv(springRadius,crossSectionRadius,1,numberOfCoils,nominalLength,type,diffuseColor,transparency);
        coilspringOpenMBV=ombv.createOpenMBV();
      }

      /** \brief Visualize a force arrow acting on each of both connected frames */
     BOOST_PARAMETER_MEMBER_FUNCTION( (void), enableOpenMBVForce, tag, (optional (scaleLength,(double),1)(scaleSize,(double),1)(referencePoint,(OpenMBV::Arrow::ReferencePoint),OpenMBV::Arrow::toPoint)(diffuseColor,(const fmatvec::Vec3&),"[-1;1;1]")(transparency,(double),0))) { 
        OpenMBVArrow ombv(diffuseColor,transparency,OpenMBV::Arrow::toHead,referencePoint,scaleLength,scaleSize);
        std::vector<bool> which; which.resize(2, true);
        LinkMechanics::setOpenMBVForceArrow(ombv.createOpenMBV(), which);
      }
#endif
    private:
      std::string saved_ref1, saved_ref2;
  };

  class GeneralizedSpringDamper : public LinkMechanics {
    protected:
      Function<double(double,double)> *func;
      std::vector<RigidBody*> body;
#ifdef HAVE_OPENMBVCPPINTERFACE
      boost::shared_ptr<OpenMBV::CoilSpring> coilspringOpenMBV;
#endif
    public:
      GeneralizedSpringDamper(const std::string &name="");
      ~GeneralizedSpringDamper();
      void updateh(double, int i=0);
      void updateg(double);
      void updategd(double);

      bool isActive() const { return true; }
      bool gActiveChanged() { return false; }
      virtual bool isSingleValued() const { return true; }
      std::string getType() const { return "GeneralizedSpringDamper"; }
      void init(InitStage stage);

      /** \brief Set the function for the generalized force. */
      void setGeneralizedForceFunction(Function<double(double,double)> *func_) {
        func=func_;
        func->setParent(this);
        func->setName("GeneralizedFoce");
      }

      void setRigidBodyFirstSide(RigidBody* body_) { body[0] = body_; }
      void setRigidBodySecondSide(RigidBody* body_) { body[1] = body_; }

      void plot(double t, double dt=1);
      void initializeUsingXML(xercesc::DOMElement *element);

      void updatehRef(const fmatvec::Vec &hParent, int j=0);

#ifdef HAVE_OPENMBVCPPINTERFACE
      BOOST_PARAMETER_MEMBER_FUNCTION( (void), enableOpenMBVCoilSpring, tag, (optional (numberOfCoils,(int),3)(springRadius,(double),1)(crossSectionRadius,(double),-1)(nominalLength,(double),-1)(type,(OpenMBV::CoilSpring::Type),OpenMBV::CoilSpring::tube)(diffuseColor,(const fmatvec::Vec3&),"[-1;1;1]")(transparency,(double),0))) { 
        OpenMBVCoilSpring ombv(springRadius,crossSectionRadius,1,numberOfCoils,nominalLength,type,diffuseColor,transparency);
        coilspringOpenMBV=ombv.createOpenMBV();
      }

      /** \brief Visualize a force arrow acting on each of both connected frames */
      BOOST_PARAMETER_MEMBER_FUNCTION( (void), enableOpenMBVForce, tag, (optional (scaleLength,(double),1)(scaleSize,(double),1)(referencePoint,(OpenMBV::Arrow::ReferencePoint),OpenMBV::Arrow::toPoint)(diffuseColor,(const fmatvec::Vec3&),"[-1;1;1]")(transparency,(double),0))) { 
        OpenMBVArrow ombv(diffuseColor,transparency,OpenMBV::Arrow::toHead,referencePoint,scaleLength,scaleSize);
        std::vector<bool> which; which.resize(2, true);
        LinkMechanics::setOpenMBVForceArrow(ombv.createOpenMBV(), which);
      }

      /** \brief Visualize a torque arrow acting on each of both connected frames */
      BOOST_PARAMETER_MEMBER_FUNCTION( (void), enableOpenMBVMoment, tag, (optional (scaleLength,(double),1)(scaleSize,(double),1)(referencePoint,(OpenMBV::Arrow::ReferencePoint),OpenMBV::Arrow::toPoint)(diffuseColor,(const fmatvec::Vec3&),"[-1;1;1]")(transparency,(double),0))) { 
        OpenMBVArrow ombv(diffuseColor,transparency,OpenMBV::Arrow::toDoubleHead,referencePoint,scaleLength,scaleSize);
        std::vector<bool> which; which.resize(2, true);
        LinkMechanics::setOpenMBVMomentArrow(ombv.createOpenMBV(), which);
      }
#endif
    private:
      std::string saved_body1, saved_body2;
  };

}

#endif /* _SPRINGDAMPER_H_ */

