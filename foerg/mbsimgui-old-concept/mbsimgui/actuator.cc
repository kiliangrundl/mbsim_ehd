/*
   MBSimGUI - A fronted for MBSim.
   Copyright (C) 2013 Martin Förg

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
   */

#include <config.h>
#include "actuator.h"
#include "ombv_properties.h"
#include "kinetics_properties.h"

using namespace std;
using namespace MBXMLUtils;

Actuator::Actuator(const string &str, Element *parent) : Link(str, parent), forceDir(0,false), momentDir(0,false), frameOfReference(0,false), actuatorForceArrow(0,false), actuatorMomentArrow(0,false) {

  vector<PhysicalVariableProperty> input;
  input.push_back(PhysicalVariableProperty(new MatProperty(3,1),"-",MBSIMCONTROLNS"forceDirection"));
  forceDir.setProperty(new ExtPhysicalVarProperty(input));

  input.clear();
  input.push_back(PhysicalVariableProperty(new MatProperty(3,1),"-",MBSIMCONTROLNS"momentDirection"));
  momentDir.setProperty(new ExtPhysicalVarProperty(input));

  frameOfReference.setProperty(new IntegerProperty(1,MBSIMCONTROLNS"referenceFrame")); 

  inputSignal.setProperty(new SignalOfReferenceProperty("",this, MBSIMCONTROLNS"inputSignal"));
  
  connections.setProperty(new ConnectFramesProperty(2,this,MBSIMCONTROLNS"connect"));

  actuatorForceArrow.setProperty(new OMBVArrowProperty("NOTSET",getID()));
  actuatorForceArrow.setXMLName(MBSIMCONTROLNS"openMBVActuatorForceArrow",false);

  actuatorMomentArrow.setProperty(new OMBVArrowProperty("NOTSET",getID()));
  actuatorMomentArrow.setXMLName(MBSIMCONTROLNS"openMBVActuatorMomentArrow",false);
}


Actuator::~Actuator() {
}

void Actuator::initialize() {
  Link::initialize();
  inputSignal.initialize();
  connections.initialize();
}

void Actuator::initializeUsingXML(TiXmlElement *element) {
  Link::initializeUsingXML(element);
  forceDir.initializeUsingXML(element);
  momentDir.initializeUsingXML(element);
  frameOfReference.initializeUsingXML(element);
  inputSignal.initializeUsingXML(element);
  connections.initializeUsingXML(element);
  actuatorForceArrow.initializeUsingXML(element);
  actuatorMomentArrow.initializeUsingXML(element);
}

TiXmlElement* Actuator::writeXMLFile(TiXmlNode *parent) {
  TiXmlElement *ele0 = Link::writeXMLFile(parent);
  forceDir.writeXMLFile(ele0);
  momentDir.writeXMLFile(ele0);
  frameOfReference.writeXMLFile(ele0);
  inputSignal.writeXMLFile(ele0);
  connections.writeXMLFile(ele0);
  actuatorForceArrow.writeXMLFile(ele0);
  actuatorMomentArrow.writeXMLFile(ele0);
  return ele0;
}

