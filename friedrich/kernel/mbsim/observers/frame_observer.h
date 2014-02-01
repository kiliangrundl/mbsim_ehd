/* Copyright (C) 2004-2013 MBSim Development Team
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
 * Contact: martin.o.foerg@gmail.com
 */

#ifndef _FRAME_OBSERVER_H__
#define _FRAME_OBSERVER_H__
#include "mbsim/observer.h"

#ifdef HAVE_OPENMBVCPPINTERFACE
#include <mbsim/utils/openmbv_utils.h>
#endif

namespace MBSim {
  class Frame;

  class FrameObserver : public Observer {
    private:
      Frame* frame;
      std::string saved_frame;
#ifdef HAVE_OPENMBVCPPINTERFACE
      OpenMBV::Arrow *openMBVPosition, *openMBVVelocity, *openMBVAngularVelocity, *openMBVAcceleration, *openMBVAngularAcceleration;
#endif

    public:
      FrameObserver(const std::string &name);
      void setFrame(Frame *frame_) { frame = frame_; } 

      void init(InitStage stage);
      virtual void plot(double t, double dt);
      virtual void initializeUsingXML(MBXMLUtils::TiXmlElement *element);

#ifdef HAVE_OPENMBVCPPINTERFACE
//      BOOST_PARAMETER_MEMBER_FUNCTION( (void), enableOpenMBVPosition, tag, (optional (diffuseColor,(const fmatvec::Vec3&),"[-1;1;1]")(transparency,(double),0)(referencePoint,(OpenMBV::Arrow::ReferencePoint),OpenMBV::Arrow::fromPoint)(scaleLength,(double),1)(scaleSize,(double),1))) { openMBVPosition=enableOpenMBVArrow(diffuseColor,transparency,OpenMBV::Arrow::toHead,referencePoint,scaleLength,scaleSize); }
//      BOOST_PARAMETER_MEMBER_FUNCTION( (void), enableOpenMBVVelocity, tag, (optional (diffuseColor,(const fmatvec::Vec3&),"[-1;1;1]")(transparency,(double),0)(referencePoint,(OpenMBV::Arrow::ReferencePoint),OpenMBV::Arrow::fromPoint)(scaleLength,(double),1)(scaleSize,(double),1))) { openMBVVelocity=enableOpenMBVArrow(diffuseColor,transparency,OpenMBV::Arrow::toHead,referencePoint,scaleLength,scaleSize); }
//      BOOST_PARAMETER_MEMBER_FUNCTION( (void), enableOpenMBVAngularVelocity, tag, (optional (diffuseColor,(const fmatvec::Vec3&),"[-1;1;1]")(transparency,(double),0)(referencePoint,(OpenMBV::Arrow::ReferencePoint),OpenMBV::Arrow::fromPoint)(scaleLength,(double),1)(scaleSize,(double),1))) { openMBVAngularVelocity=enableOpenMBVArrow(diffuseColor,transparency,OpenMBV::Arrow::toDoubleHead,referencePoint,scaleLength,scaleSize); }
//      BOOST_PARAMETER_MEMBER_FUNCTION( (void), enableOpenMBVAcceleration, tag, (optional (diffuseColor,(const fmatvec::Vec3&),"[-1;1;1]")(transparency,(double),0)(referencePoint,(OpenMBV::Arrow::ReferencePoint),OpenMBV::Arrow::fromPoint)(scaleLength,(double),1)(scaleSize,(double),1))) { openMBVAcceleration=enableOpenMBVArrow(diffuseColor,transparency,OpenMBV::Arrow::toHead,referencePoint,scaleLength,scaleSize); }
//      BOOST_PARAMETER_MEMBER_FUNCTION( (void), enableOpenMBVAngularAcceleration, tag, (optional (diffuseColor,(const fmatvec::Vec3&),"[-1;1;1]")(transparency,(double),0)(referencePoint,(OpenMBV::Arrow::ReferencePoint),OpenMBV::Arrow::fromPoint)(scaleLength,(double),1)(scaleSize,(double),1))) { openMBVAngularAcceleration=enableOpenMBVArrow(diffuseColor,transparency,OpenMBV::Arrow::toDoubleHead,referencePoint,scaleLength,scaleSize); }
#endif

  };

}  

#endif
