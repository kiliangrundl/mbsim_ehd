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

#include <config.h>
#include "mbsim/observers/frame_observer.h"
#include "mbsim/frame.h"

using namespace std;
using namespace fmatvec;

namespace MBSim {

  FrameObserver::FrameObserver(const std::string &name) : Observer(name), frame(0) {
#ifdef HAVE_OPENMBVCPPINTERFACE
    openMBVPositionArrow=0;
    openMBVVelocityArrow=0;
    openMBVAngularVelocityArrow=0;
    openMBVAccelerationArrow=0;
    openMBVAngularAccelerationArrow=0;
#endif
  }

  void FrameObserver::init(InitStage stage) {
    if(stage==MBSim::plot) {
      updatePlotFeatures();

      Observer::init(stage);
      if(getPlotFeature(plotRecursive)==enabled) {
#ifdef HAVE_OPENMBVCPPINTERFACE
        if(getPlotFeature(openMBV)==enabled) {
          if(openMBVPositionArrow) {
            openMBVPositionArrow->setName("Position");
            getOpenMBVGrp()->addObject(openMBVPositionArrow);
          }
          if(openMBVVelocityArrow) {
            openMBVVelocityArrow->setName("Velocity");
            getOpenMBVGrp()->addObject(openMBVVelocityArrow);
          }
          if(openMBVAngularVelocityArrow) {
            openMBVAngularVelocityArrow->setName("AngularVelocity");
            getOpenMBVGrp()->addObject(openMBVAngularVelocityArrow);
          }
          if(openMBVAccelerationArrow) {
            openMBVAccelerationArrow->setName("Acceleration");
            getOpenMBVGrp()->addObject(openMBVAccelerationArrow);
          }
          if(openMBVAngularAccelerationArrow) {
            openMBVAngularAccelerationArrow->setName("AngularAcceleration");
            getOpenMBVGrp()->addObject(openMBVAngularAccelerationArrow);
          }
        }
#endif
      }
    }
    else
      Observer::init(stage);
  }

#ifdef HAVE_OPENMBVCPPINTERFACE
  void FrameObserver::enableOpenMBVPosition(double diameter, double headDiameter, double headLength, double color) {
    openMBVPositionArrow=new OpenMBV::Arrow;
    openMBVPositionArrow->setReferencePoint(OpenMBV::Arrow::fromPoint);
    openMBVPositionArrow->setDiameter(diameter);
    openMBVPositionArrow->setHeadDiameter(headDiameter);
    openMBVPositionArrow->setHeadLength(headLength);
    openMBVPositionArrow->setStaticColor(color);
  }
  void FrameObserver::enableOpenMBVVelocity(double scale, OpenMBV::Arrow::ReferencePoint refPoint, double diameter, double headDiameter, double headLength, double color) {
    openMBVVelocityArrow=new OpenMBV::Arrow;
    openMBVVelocityArrow->setScaleLength(scale);
    openMBVVelocityArrow->setReferencePoint(refPoint);
    openMBVVelocityArrow->setDiameter(diameter);
    openMBVVelocityArrow->setHeadDiameter(headDiameter);
    openMBVVelocityArrow->setHeadLength(headLength);
    openMBVVelocityArrow->setStaticColor(color);
  }
  void FrameObserver::enableOpenMBVAngularVelocity(double scale, OpenMBV::Arrow::ReferencePoint refPoint, double diameter, double headDiameter, double headLength, double color) {
    openMBVAngularVelocityArrow=new OpenMBV::Arrow;
    openMBVAngularVelocityArrow->setScaleLength(scale);
    openMBVAngularVelocityArrow->setType(OpenMBV::Arrow::toDoubleHead);
    openMBVAngularVelocityArrow->setReferencePoint(refPoint);
    openMBVAngularVelocityArrow->setDiameter(diameter);
    openMBVAngularVelocityArrow->setHeadDiameter(headDiameter);
    openMBVAngularVelocityArrow->setHeadLength(headLength);
    openMBVAngularVelocityArrow->setStaticColor(color);
  }
  void FrameObserver::enableOpenMBVAcceleration(double scale, OpenMBV::Arrow::ReferencePoint refPoint, double diameter, double headDiameter, double headLength, double color) {
    openMBVAccelerationArrow=new OpenMBV::Arrow;
    openMBVAccelerationArrow->setScaleLength(scale);
    openMBVAccelerationArrow->setReferencePoint(refPoint);
    openMBVAccelerationArrow->setDiameter(diameter);
    openMBVAccelerationArrow->setHeadDiameter(headDiameter);
    openMBVAccelerationArrow->setHeadLength(headLength);
    openMBVAccelerationArrow->setStaticColor(color);
  }
  void FrameObserver::enableOpenMBVAngularAcceleration(double scale, OpenMBV::Arrow::ReferencePoint refPoint, double diameter, double headDiameter, double headLength, double color) {
    openMBVAngularAccelerationArrow=new OpenMBV::Arrow;
    openMBVAngularAccelerationArrow->setScaleLength(scale);
    openMBVAngularAccelerationArrow->setType(OpenMBV::Arrow::toDoubleHead);
    openMBVAngularAccelerationArrow->setReferencePoint(refPoint);
    openMBVAngularAccelerationArrow->setDiameter(diameter);
    openMBVAngularAccelerationArrow->setHeadDiameter(headDiameter);
    openMBVAngularAccelerationArrow->setHeadLength(headLength);
    openMBVAngularAccelerationArrow->setStaticColor(color);
  }
#endif

  void FrameObserver::plot(double t, double dt) {
    if(getPlotFeature(plotRecursive)==enabled) {
#ifdef HAVE_OPENMBVCPPINTERFACE
      if(getPlotFeature(openMBV)==enabled) {
        if(openMBVPositionArrow && !openMBVPositionArrow->isHDF5Link()) {
          vector<double> data;
          data.push_back(t);
          data.push_back(0);
          data.push_back(0);
          data.push_back(0);
          data.push_back(frame->getPosition()(0));
          data.push_back(frame->getPosition()(1));
          data.push_back(frame->getPosition()(2));
          data.push_back(0.5);
          openMBVPositionArrow->append(data);
        }
        if(openMBVVelocityArrow && !openMBVVelocityArrow->isHDF5Link()) {
          vector<double> data;
          data.push_back(t);
          data.push_back(frame->getPosition()(0));
          data.push_back(frame->getPosition()(1));
          data.push_back(frame->getPosition()(2));
          data.push_back(frame->getVelocity()(0));
          data.push_back(frame->getVelocity()(1));
          data.push_back(frame->getVelocity()(2));
          data.push_back(0.5);
          openMBVVelocityArrow->append(data);
        }
        if(openMBVAngularVelocityArrow && !openMBVAngularVelocityArrow->isHDF5Link()) {
          vector<double> data;
          data.push_back(t);
          data.push_back(frame->getPosition()(0));
          data.push_back(frame->getPosition()(1));
          data.push_back(frame->getPosition()(2));
          data.push_back(frame->getAngularVelocity()(0));
          data.push_back(frame->getAngularVelocity()(1));
          data.push_back(frame->getAngularVelocity()(2));
          data.push_back(0.5);
          openMBVAngularVelocityArrow->append(data);
        }
        if(openMBVAccelerationArrow && !openMBVAccelerationArrow->isHDF5Link()) {
          vector<double> data;
          data.push_back(t);
          data.push_back(frame->getPosition()(0));
          data.push_back(frame->getPosition()(1));
          data.push_back(frame->getPosition()(2));
          data.push_back(frame->getAcceleration()(0));
          data.push_back(frame->getAcceleration()(1));
          data.push_back(frame->getAcceleration()(2));
          data.push_back(0.5);
          openMBVAccelerationArrow->append(data);
        }
        if(openMBVAngularAccelerationArrow && !openMBVAngularAccelerationArrow->isHDF5Link()) {
          vector<double> data;
          data.push_back(t);
          data.push_back(frame->getPosition()(0));
          data.push_back(frame->getPosition()(1));
          data.push_back(frame->getPosition()(2));
          data.push_back(frame->getAngularAcceleration()(0));
          data.push_back(frame->getAngularAcceleration()(1));
          data.push_back(frame->getAngularAcceleration()(2));
          data.push_back(0.5);
          openMBVAngularAccelerationArrow->append(data);
        }
      }
#endif

      Observer::plot(t,dt);
    }
  }

}