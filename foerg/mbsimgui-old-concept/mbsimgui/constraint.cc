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
#include "constraint.h"
#include "frame.h"
#include "rigidbody.h"
#include "basic_properties.h"
#include "kinetics_properties.h"
#include "function_properties.h"
#include "function_property_factory.h"
#include "ombv_properties.h"

using namespace std;
using namespace MBXMLUtils;

class RigidBodyOfReferencePropertyFactory : public PropertyFactory {
  public:
    RigidBodyOfReferencePropertyFactory(Element *element_, const string &xmlName_) : element(element_), xmlName(xmlName_) { }
    Property* createProperty(int i=0);
  protected:
    Element *element;
    string xmlName;
};

Property* RigidBodyOfReferencePropertyFactory::createProperty(int i) {
 return new RigidBodyOfReferenceProperty("",element,xmlName);
}

class GearConstraintPropertyFactory : public PropertyFactory {
  public:
    GearConstraintPropertyFactory(Element *element_, const string &xmlName_) : element(element_), xmlName(xmlName_) { }
    Property* createProperty(int i=0);
  protected:
    Element *element;
    string xmlName;
};

Property* GearConstraintPropertyFactory::createProperty(int i) {
  ContainerProperty *property = new ContainerProperty;
  property->addProperty(new RigidBodyOfReferenceProperty("",element,xmlName));
  vector<PhysicalVariableProperty> input;
  input.push_back(PhysicalVariableProperty(new ScalarProperty("1"), "", MBSIMNS"transmissionRatio"));
  property->addProperty(new ExtProperty(new ExtPhysicalVarProperty(input)));
  return property;
}

Constraint::Constraint(const string &str, Element *parent) : Object(str, parent) {
}

GearConstraint::GearConstraint(const string &str, Element *parent) : Constraint(str, parent), gearForceArrow(0,false), gearMomentArrow(0,false) {

  dependentBody.setProperty(new RigidBodyOfReferenceProperty("",this,MBSIMNS"dependentRigidBody"));

  //independentBodies.setProperty(new GearDependenciesProperty(this,MBSIMNS"independentRigidBodies"));
  independentBodies.setProperty(new ListProperty(new GearConstraintPropertyFactory(this,""),MBSIMNS"independentRigidBody"));
  independentBodies.setXMLName(MBSIMNS"independentRigidBodies");

  gearForceArrow.setProperty(new OMBVArrowProperty("NOTSET",getID()));
  gearForceArrow.setXMLName(MBSIMNS"openMBVGearForceArrow",false);

  gearMomentArrow.setProperty(new OMBVArrowProperty("NOTSET",getID()));
  gearMomentArrow.setXMLName(MBSIMNS"openMBVGearMomentArrow",false);
}

void GearConstraint::initialize() {
  Constraint::initialize();
  dependentBody.initialize();
  independentBodies.initialize();
  RigidBody *body = static_cast<RigidBodyOfReferenceProperty*>(dependentBody.getProperty())->getBodyPtr();
  if(body)
    body->setConstrained(true);
}

void GearConstraint::deinitialize() {
  Constraint::deinitialize();
  RigidBody *body = static_cast<RigidBodyOfReferenceProperty*>(dependentBody.getProperty())->getBodyPtr();
  if(body)
    body->setConstrained(false);
}

void GearConstraint::initializeUsingXML(TiXmlElement *element) {
  TiXmlElement *e, *ee;
  Constraint::initializeUsingXML(element);
  dependentBody.initializeUsingXML(element);
  independentBodies.initializeUsingXML(element);
  gearForceArrow.initializeUsingXML(element);
  gearMomentArrow.initializeUsingXML(element);
}

TiXmlElement* GearConstraint::writeXMLFile(TiXmlNode *parent) {
  TiXmlElement *ele0 = Constraint::writeXMLFile(parent);
  dependentBody.writeXMLFile(ele0);
  independentBodies.writeXMLFile(ele0);
  gearForceArrow.writeXMLFile(ele0);
  gearMomentArrow.writeXMLFile(ele0);
  return ele0;
}

KinematicConstraint::KinematicConstraint(const string &str, Element *parent) : Constraint(str, parent), constraintForceArrow(0,false), constraintMomentArrow(0,false) {

  dependentBody.setProperty(new RigidBodyOfReferenceProperty("",this,MBSIMNS"dependentRigidBody"));

  constraintForceArrow.setProperty(new OMBVArrowProperty("NOTSET",getID()));
  constraintForceArrow.setXMLName(MBSIMNS"openMBVConstraintForceArrow",false);

  constraintMomentArrow.setProperty(new OMBVArrowProperty("NOTSET",getID()));
  constraintMomentArrow.setXMLName(MBSIMNS"openMBVConstraintMomentArrow",false);
}

void KinematicConstraint::initialize() {
  Constraint::initialize();
  dependentBody.initialize();
  RigidBody *body = static_cast<RigidBodyOfReferenceProperty*>(dependentBody.getProperty())->getBodyPtr();
  if(body)
    body->setConstrained(true);
}

void KinematicConstraint::deinitialize() {
  Constraint::deinitialize();
  RigidBody *body = static_cast<RigidBodyOfReferenceProperty*>(dependentBody.getProperty())->getBodyPtr();
  if(body)
    body->setConstrained(false);
}

void KinematicConstraint::initializeUsingXML(TiXmlElement *element) {
  TiXmlElement *e, *ee;
  Constraint::initializeUsingXML(element);
  dependentBody.initializeUsingXML(element);
  constraintForceArrow.initializeUsingXML(element);
  constraintMomentArrow.initializeUsingXML(element);
}

TiXmlElement* KinematicConstraint::writeXMLFile(TiXmlNode *parent) {
  TiXmlElement *ele0 = Constraint::writeXMLFile(parent);

  dependentBody.writeXMLFile(ele0);
  constraintForceArrow.writeXMLFile(ele0);
  constraintMomentArrow.writeXMLFile(ele0);

  return ele0;
}

GeneralizedPositionConstraint::GeneralizedPositionConstraint(const string &str, Element *parent) : KinematicConstraint(str, parent), constraintFunction(0,false) {

  constraintFunction.setProperty(new ChoiceProperty2(new FunctionPropertyFactory2,MBSIMNS"constraintFunction"));
}

void GeneralizedPositionConstraint::initializeUsingXML(TiXmlElement *element) {
  TiXmlElement *e, *ee;
  KinematicConstraint::initializeUsingXML(element);
  constraintFunction.initializeUsingXML(element);
}

TiXmlElement* GeneralizedPositionConstraint::writeXMLFile(TiXmlNode *parent) {
  TiXmlElement *ele0 = KinematicConstraint::writeXMLFile(parent);

  constraintFunction.writeXMLFile(ele0);

  return ele0;
}

GeneralizedVelocityConstraint::GeneralizedVelocityConstraint(const string &str, Element *parent) : KinematicConstraint(str, parent), constraintFunction(0,false), x0(0,false) {

  constraintFunction.setProperty(new ChoiceProperty2(new ConstraintPropertyFactory,"",3)); 

  vector<PhysicalVariableProperty> input;
  input.push_back(PhysicalVariableProperty(new VecProperty(0),"",MBSIMNS"initialState"));
  x0.setProperty(new ExtPhysicalVarProperty(input));
}

void GeneralizedVelocityConstraint::initializeUsingXML(TiXmlElement *element) {
  TiXmlElement *e, *ee;
  KinematicConstraint::initializeUsingXML(element);
  x0.initializeUsingXML(element);
  constraintFunction.initializeUsingXML(element);
}

TiXmlElement* GeneralizedVelocityConstraint::writeXMLFile(TiXmlNode *parent) {
  TiXmlElement *ele0 = KinematicConstraint::writeXMLFile(parent);

  x0.writeXMLFile(ele0);
  constraintFunction.writeXMLFile(ele0);

  return ele0;
}

GeneralizedAccelerationConstraint::GeneralizedAccelerationConstraint(const string &str, Element *parent) : KinematicConstraint(str, parent), constraintFunction(0,false), x0(0,false) {

  constraintFunction.setProperty(new ChoiceProperty2(new ConstraintPropertyFactory,"",3)); 

  vector<PhysicalVariableProperty> input;
  input.push_back(PhysicalVariableProperty(new VecProperty(0),"",MBSIMNS"initialState"));
  x0.setProperty(new ExtPhysicalVarProperty(input));
}

void GeneralizedAccelerationConstraint::initializeUsingXML(TiXmlElement *element) {
  TiXmlElement *e, *ee;
  KinematicConstraint::initializeUsingXML(element);
  x0.initializeUsingXML(element);
  constraintFunction.initializeUsingXML(element);
}

TiXmlElement* GeneralizedAccelerationConstraint::writeXMLFile(TiXmlNode *parent) {
  TiXmlElement *ele0 = KinematicConstraint::writeXMLFile(parent);

  x0.writeXMLFile(ele0);
  constraintFunction.writeXMLFile(ele0);

  return ele0;
}

JointConstraint::JointConstraint(const string &str, Element *parent) : Constraint(str, parent), refFrameID(0,false), force(0,false), moment(0,false), jointForceArrow(0,false), jointMomentArrow(0,false), q0(0,false) {

  independentBody.setProperty(new RigidBodyOfReferenceProperty("",this,MBSIMNS"independentRigidBody"));

  dependentBodiesFirstSide.setProperty(new ListProperty(new RigidBodyOfReferencePropertyFactory(this,""),MBSIMNS"dependentRigidBody"));
  dependentBodiesFirstSide.setXMLName(MBSIMNS"dependentRigidBodiesFirstSide");

  dependentBodiesSecondSide.setProperty(new ListProperty(new RigidBodyOfReferencePropertyFactory(this,""),MBSIMNS"dependentRigidBody"));
  dependentBodiesSecondSide.setXMLName(MBSIMNS"dependentRigidBodiesSecondSide");

  refFrameID.setProperty(new IntegerProperty(0,MBSIMNS"frameOfReferenceID"));

  vector<PhysicalVariableProperty> input;
  input.push_back(PhysicalVariableProperty(new MatProperty(3,1),"-",MBSIMNS"forceDirection"));
  force.setProperty(new ExtPhysicalVarProperty(input));

  input.clear();
  input.push_back(PhysicalVariableProperty(new MatProperty(3,1),"-",MBSIMNS"momentDirection"));
  moment.setProperty(new ExtPhysicalVarProperty(input));

  connections.setProperty(new ConnectFramesProperty(2,this));

  jointForceArrow.setProperty(new OMBVArrowProperty("NOTSET",getID()));
  jointForceArrow.setXMLName(MBSIMNS"openMBVJointForceArrow",false);

  jointMomentArrow.setProperty(new OMBVArrowProperty("NOTSET",getID()));
  jointMomentArrow.setXMLName(MBSIMNS"openMBVJointMomentArrow",false);

  input.clear();
  input.push_back(PhysicalVariableProperty(new VecProperty(0),"",MBSIMNS"initialGuess"));
  q0.setProperty(new ExtPhysicalVarProperty(input));
}

void JointConstraint::initialize() {
  Constraint::initialize();
  independentBody.initialize();
  dependentBodiesFirstSide.initialize();
  dependentBodiesSecondSide.initialize();
  connections.initialize();
  ListProperty *list = static_cast<ListProperty*>(dependentBodiesFirstSide.getProperty());
  for(int i=0; i<list->getSize(); i++) {
    RigidBody *body = static_cast<RigidBodyOfReferenceProperty*>(list->getProperty(i))->getBodyPtr();
    if(body)
      body->setConstrained(true);
  }
  list = static_cast<ListProperty*>(dependentBodiesSecondSide.getProperty());
  for(int i=0; i<list->getSize(); i++) {
    RigidBody *body = static_cast<RigidBodyOfReferenceProperty*>(list->getProperty(i))->getBodyPtr();
    if(body)
      body->setConstrained(true);
  }
}

void JointConstraint::deinitialize() {
  Constraint::deinitialize();
  ListProperty *list = static_cast<ListProperty*>(dependentBodiesFirstSide.getProperty());
  for(int i=0; i<list->getSize(); i++) {
    RigidBody *body = static_cast<RigidBodyOfReferenceProperty*>(list->getProperty(i))->getBodyPtr();
    if(body)
      body->setConstrained(false);
  }
  list = static_cast<ListProperty*>(dependentBodiesSecondSide.getProperty());
  for(int i=0; i<list->getSize(); i++) {
    RigidBody *body = static_cast<RigidBodyOfReferenceProperty*>(list->getProperty(i))->getBodyPtr();
    if(body)
      body->setConstrained(false);
  }
}

void JointConstraint::initializeUsingXML(TiXmlElement *element) {
  Constraint::initializeUsingXML(element);

  q0.initializeUsingXML(element);

  dependentBodiesFirstSide.initializeUsingXML(element);
  dependentBodiesSecondSide.initializeUsingXML(element);

  independentBody.initializeUsingXML(element);

  refFrameID.initializeUsingXML(element);
  force.initializeUsingXML(element);
  moment.initializeUsingXML(element);

  connections.initializeUsingXML(element);

  jointForceArrow.initializeUsingXML(element);
  jointMomentArrow.initializeUsingXML(element);
}

TiXmlElement* JointConstraint::writeXMLFile(TiXmlNode *parent) {
  TiXmlElement *ele0 = Constraint::writeXMLFile(parent);

  q0.writeXMLFile(ele0);

  dependentBodiesFirstSide.writeXMLFile(ele0);
  dependentBodiesSecondSide.writeXMLFile(ele0);

  independentBody.writeXMLFile(ele0);

  refFrameID.writeXMLFile(ele0);
  force.writeXMLFile(ele0);
  moment.writeXMLFile(ele0);

  connections.writeXMLFile(ele0);

  jointForceArrow.writeXMLFile(ele0);
  jointMomentArrow.writeXMLFile(ele0);

  return ele0;
}