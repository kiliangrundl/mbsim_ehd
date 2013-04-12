/*
    MBSimGUI - A fronted for MBSim.
    Copyright (C) 2012 Martin Förg

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
#include "objectfactory.h"
#include "frame.h"
#include "contour.h"
#include "solver.h"
#include "group.h"
#include "rigidbody.h"
#include "constraint.h"
#include "kinetic_excitation.h"
#include "joint.h"
#include "spring_damper.h"
#include "contact.h"
#include "signal_.h"
#include "widget.h"
#include "parameter.h"
#include "observer.h"
#include "integrator.h"
#include <string>

using namespace std;

ObjectFactory* ObjectFactory::instance=NULL;

Environment *ObjectFactory::getEnvironment(TiXmlElement *element) {
  if(element==NULL) return NULL;
  Environment *obj;
  for(set<ObjectFactoryBase*>::iterator i=factories.begin(); i!=factories.end(); i++)
    if((obj=(*i)->getEnvironment(element))) return obj;
  cout << string("No Environment of type ")+element->ValueStr()+" exists.";
  throw;
}

Environment *MBSimObjectFactory::getEnvironment(TiXmlElement *element) {
  if(element==0) return 0;
  if(element->ValueStr()==MBSIMNS"MBSimEnvironment")
    return Environment::getInstance();
  return 0;
}

MBSimObjectFactory *MBSimObjectFactory::instance=NULL;

void MBSimObjectFactory::initialize() {
  if(instance==0) {
    instance=new MBSimObjectFactory;
    ObjectFactory::getInstance()->registerObjectFactory(instance);
  }
}

Contour* ObjectFactory::createContour(TiXmlElement *element, Element *parent) {
  if(element==NULL) return NULL;
  for(set<ObjectFactoryBase*>::iterator i=factories.begin(); i!=factories.end(); i++)
    return (*i)->createContour(element,parent);
  return 0;
}
Contour* MBSimObjectFactory::createContour(TiXmlElement *element, Element *parent) {
  if(element==0) return 0;
  if(element->ValueStr()==MBSIMNS"Point")
    return new Point(element->Attribute("name"),parent);
  else if(element->ValueStr()==MBSIMNS"Line")
    return new Line(element->Attribute("name"),parent);
  else if(element->ValueStr()==MBSIMNS"Plane")
    return new Plane(element->Attribute("name"),parent);
  else if(element->ValueStr()==MBSIMNS"Sphere")
    return new Sphere(element->Attribute("name"),parent);
  return 0;
}

Group* ObjectFactory::createGroup(TiXmlElement *element, Element *parent) {
  if(element==NULL) return NULL;
  //Group *obj;
  for(set<ObjectFactoryBase*>::iterator i=factories.begin(); i!=factories.end(); i++)
    return (((*i)->createGroup(element,parent)));
  return 0;
}
Group* MBSimObjectFactory::createGroup(TiXmlElement *element, Element *parent) {
  if(element==0) return 0;
  if(element->ValueStr()==MBSIMNS"DynamicSystemSolver")
    return new Solver(element->Attribute("name"),parent);
  else if(element->ValueStr()==MBSIMNS"Group")
    return new Group(element->Attribute("name"),parent);
  return 0;
}

Object* ObjectFactory::createObject(TiXmlElement *element, Element *parent) {
  if(element==NULL) return NULL;
  for(set<ObjectFactoryBase*>::iterator i=factories.begin(); i!=factories.end(); i++)
    return (*i)->createObject(element,parent);
  return 0;
}
Object* MBSimObjectFactory::createObject(TiXmlElement *element, Element *parent) {
  if(element==0) return 0;
  if(element->ValueStr()==MBSIMNS"RigidBody")
    return new RigidBody(element->Attribute("name"),parent);
  else if(element->ValueStr()==MBSIMNS"GearConstraint")
    return new GearConstraint(element->Attribute("name"),parent);
  else if(element->ValueStr()==MBSIMNS"KinematicConstraint")
    return new KinematicConstraint(element->Attribute("name"),parent);
  else if(element->ValueStr()==MBSIMNS"JointConstraint")
    return new JointConstraint(element->Attribute("name"),parent);
  return 0;
}

Link* ObjectFactory::createLink(TiXmlElement *element, Element *parent) {
  if(element==NULL) return NULL;
  for(set<ObjectFactoryBase*>::iterator i=factories.begin(); i!=factories.end(); i++)
    return (*i)->createLink(element,parent);
  return 0;
}
Link* MBSimObjectFactory::createLink(TiXmlElement *element, Element *parent) {
  if(element==0) return 0;
  if(element->ValueStr()==MBSIMNS"KineticExcitation")
    return new KineticExcitation(element->Attribute("name"),parent);
  if(element->ValueStr()==MBSIMNS"SpringDamper")
    return new SpringDamper(element->Attribute("name"),parent);
  if(element->ValueStr()==MBSIMNS"Joint")
    return new Joint(element->Attribute("name"),parent);
  if(element->ValueStr()==MBSIMNS"Contact")
    return new Contact(element->Attribute("name"),parent);
  //if(element->ValueStr()==MBSIMCONTROLNS"AbsolutePositionSensor")
  //  return new AbsolutePositionSensor(element->Attribute("name"),parent);
  //if(element->ValueStr()==MBSIMNS"ExternGeneralizedIO")
  //  return new ExternGeneralizedIO(element->Attribute("name"));
  return 0;
}  

Observer* ObjectFactory::createObserver(TiXmlElement *element, Element *parent) {
  if(element==NULL) return NULL;
  for(set<ObjectFactoryBase*>::iterator i=factories.begin(); i!=factories.end(); i++)
    return (*i)->createObserver(element,parent);
  return 0;
}
Observer* MBSimObjectFactory::createObserver(TiXmlElement *element, Element *parent) {
  if(element==0) return 0;
  if(element->ValueStr()==MBSIMNS"AbsoluteKinematicsObserver")
    return new AbsoluteKinematicsObserver(element->Attribute("name"),parent);
  return 0;
}  

Integrator* ObjectFactory::createIntegrator(TiXmlElement *element, QTreeWidgetItem* parentItem) {
  if(element==NULL) return NULL;
  for(set<ObjectFactoryBase*>::iterator i=factories.begin(); i!=factories.end(); i++)
    return (*i)->createIntegrator(element,parentItem);
  return 0;
}

Integrator* MBSimObjectFactory::createIntegrator(TiXmlElement *element, QTreeWidgetItem* parentItem) {
  if(element==0) return 0;
  if(element->ValueStr()==MBSIMINTNS"DOPRI5Integrator")
    return new DOPRI5Integrator("DOPRI5",parentItem);
  else if(element->ValueStr()==MBSIMINTNS"RADAU5Integrator")
    return new RADAU5Integrator("RADAU5",parentItem);
  else if(element->ValueStr()==MBSIMINTNS"LSODEIntegrator")
    return new LSODEIntegrator("LSODE",parentItem);
  else if(element->ValueStr()==MBSIMINTNS"LSODARIntegrator")
    return new LSODARIntegrator("LSODAR",parentItem);
  else if(element->ValueStr()==MBSIMINTNS"TimeSteppingIntegrator")
    return new TimeSteppingIntegrator("TimeStepping",parentItem);
  else if(element->ValueStr()==MBSIMINTNS"EulerExplicitIntegrator")
    return new EulerExplicitIntegrator("EulerExplicit",parentItem);
  else if(element->ValueStr()==MBSIMINTNS"RKSuiteIntegrator")
    return new RKSuiteIntegrator("RKSuite",parentItem);
  return 0;
}

Parameter* ObjectFactory::createParameter(TiXmlElement *element, QTreeWidgetItem* parentItem) {
  if(element==NULL) return NULL;
  for(set<ObjectFactoryBase*>::iterator i=factories.begin(); i!=factories.end(); i++)
    return (*i)->createParameter(element,parentItem);
  return 0;
}

Parameter* MBSimObjectFactory::createParameter(TiXmlElement *element, QTreeWidgetItem* parentItem) {
  if(element==0) return 0;
  if(element->ValueStr()==PARAMNS"scalarParameter")
    return new DoubleParameter(element->Attribute("name"),parentItem);
  return 0;
}

ObjectFactoryBase::M_NSPRE ObjectFactory::getNamespacePrefixMapping() {
  // collect all priority-namespace-prefix mappings
  MM_PRINSPRE priorityNSPrefix;
  for(set<ObjectFactoryBase*>::iterator i=factories.begin(); i!=factories.end(); i++)
    priorityNSPrefix.insert((*i)->getPriorityNamespacePrefix().begin(), (*i)->getPriorityNamespacePrefix().end());
#ifdef HAVE_OPENMBVCPPINTERFACE
  // add the openmbv mapping
  priorityNSPrefix.insert(OpenMBV::ObjectFactory::getPriorityNamespacePrefix().begin(),
      OpenMBV::ObjectFactory::getPriorityNamespacePrefix().end());
#endif

  // generate the namespace-prefix mapping considering the priority
  M_NSPRE nsprefix;
  set<string> prefix;
  for(MM_PRINSPRE::reverse_iterator i=priorityNSPrefix.rbegin(); i!=priorityNSPrefix.rend(); i++) {
    // insert only if the prefix does not already exist
    if(prefix.find(i->second.second)!=prefix.end())
      continue;
    // insert only if the namespace does not already exist
    pair<M_NSPRE::iterator, bool> ret=nsprefix.insert(i->second);
    if(ret.second)
      prefix.insert(i->second.second);
  }

  return nsprefix;
}


