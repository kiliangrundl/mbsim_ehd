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
 * Contact: markus.ms.schneider@gmail.com
 */

#include "mbsim/utils/function_library.h"
#include "mbsim/objectfactory.h"

using namespace std;
using namespace MBXMLUtils;
using namespace fmatvec;

namespace MBSim {

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, LinearSpringDamperForce, MBSIMNS"LinearSpringDamperForce")

  void LinearSpringDamperForce::initializeUsingXML(TiXmlElement *element) {
    Function<double(double,double)>::initializeUsingXML(element);
    TiXmlElement *e;
    e = element->FirstChildElement(MBSIMNS"stiffnessCoefficient");
    c = Element::getDouble(e);
    e = element->FirstChildElement(MBSIMNS"dampingCoefficient");
    d = Element::getDouble(e);
    e = element->FirstChildElement(MBSIMNS"unloadedLength");
    l0 = Element::getDouble(e);
  }

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, NonlinearSpringDamperForce, MBSIMNS"NonlinearSpringDamperForce")

  void NonlinearSpringDamperForce::initializeUsingXML(TiXmlElement *element) {
    Function<double(double,double)>::initializeUsingXML(element);
    TiXmlElement *e;
    e = element->FirstChildElement(MBSIMNS"distanceForce");
    gForceFun = ObjectFactory<FunctionBase>::create<Function<Vec(double)> >(e->FirstChildElement());
    gForceFun->initializeUsingXML(e->FirstChildElement());
    e = element->FirstChildElement(MBSIMNS"velocityForce");
    gdForceFun = ObjectFactory<FunctionBase>::create<Function<Vec(double)> >(e->FirstChildElement());
    gdForceFun->initializeUsingXML(e->FirstChildElement());
  }

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, LinearRegularizedUnilateralConstraint, MBSIMNS"LinearRegularizedUnilateralConstraint")

  void LinearRegularizedUnilateralConstraint::initializeUsingXML(TiXmlElement *element) {
    Function<double(double,double)>::initializeUsingXML(element);
    TiXmlElement *e;
    e = element->FirstChildElement(MBSIMNS"stiffnessCoefficient");
    c = Element::getDouble(e);
    e = element->FirstChildElement(MBSIMNS"dampingCoefficient");
    d = Element::getDouble(e);
  }

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, LinearRegularizedBilateralConstraint, MBSIMNS"LinearRegularizedBilateralConstraint")

  void LinearRegularizedBilateralConstraint::initializeUsingXML(TiXmlElement *element) {
    Function<double(double,double)>::initializeUsingXML(element);
    TiXmlElement *e;
    e = element->FirstChildElement(MBSIMNS"stiffnessCoefficient");
    c = Element::getDouble(e);
    e = element->FirstChildElement(MBSIMNS"dampingCoefficient");
    d = Element::getDouble(e);
  }

  TiXmlElement* LinearRegularizedBilateralConstraint::writeXMLFile(TiXmlNode *parent) {
    TiXmlElement *ele0 = Function<double(double,double)>::writeXMLFile(parent);
    addElementText(ele0, MBSIMNS"stiffnessCoefficient", c);
    addElementText(ele0, MBSIMNS"dampingCoefficient", d);
    return ele0;
  }

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, LinearRegularizedCoulombFriction, MBSIMNS"LinearRegularizedCoulombFriction")

  Vec LinearRegularizedCoulombFriction::operator()(const Vec &gd, const double& laN) {
    int nFric = gd.size();
    Vec la(nFric, NONINIT);
    double normgd = nrm2(gd(0, nFric - 1));
    if (normgd < gdLim)
      la(0, nFric - 1) = gd(0, nFric - 1) * (-laN * mu / gdLim);
    else
      la(0, nFric - 1) = gd(0, nFric - 1) * (-laN * mu / normgd);
    return la;
  }

  void LinearRegularizedCoulombFriction::initializeUsingXML(TiXmlElement *element) {
    Function<Vec(Vec,double)>::initializeUsingXML(element);
    TiXmlElement *e;
    e = element->FirstChildElement(MBSIMNS"marginalVelocity");
    if (e)
      gdLim = Element::getDouble(e);
    e = element->FirstChildElement(MBSIMNS"frictionCoefficient");
    mu = Element::getDouble(e);
  }

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, LinearRegularizedStribeckFriction, MBSIMNS"LinearRegularizedStribeckFriction")

  Vec LinearRegularizedStribeckFriction::operator()(const Vec &gd, const double& laN) {
    int nFric = gd.size();
    Vec la(nFric, NONINIT);
    double normgd = nrm2(gd(0, nFric - 1));
    if (normgd < gdLim) {
      double mu0 = (*fmu)(0);
      la(0, nFric - 1) = gd(0, nFric - 1) * (-laN * mu0 / gdLim);
    }
    else {
      double mu = (*fmu)(nrm2(gd(0, nFric - 1)) - gdLim);
      la(0, nFric - 1) = gd(0, nFric - 1) * (-laN * mu / normgd);
    }
    return la;
  }

  void LinearRegularizedStribeckFriction::initializeUsingXML(TiXmlElement *element) {
    Function<Vec(Vec,double)>::initializeUsingXML(element);
    TiXmlElement *e;
    e = element->FirstChildElement(MBSIMNS"marginalVelocity");
    if (e)
      gdLim = Element::getDouble(e);
    e = element->FirstChildElement(MBSIMNS"frictionFunction");
    Function<double(double)> *f = ObjectFactory<FunctionBase>::create<Function<double(double)> >(e->FirstChildElement());
    setFrictionFunction(f);
    f->initializeUsingXML(e->FirstChildElement());
  }

  void InfluenceFunction::initializeUsingXML(TiXmlElement *element) {
    Function<double(Vec2,Vec2)>::initializeUsingXML(element);
  }

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, FlexibilityInfluenceFunction, MBSIMNS"FlexibilityInfluenceFunction")

  void FlexibilityInfluenceFunction::initializeUsingXML(TiXmlElement *element) {
    InfluenceFunction::initializeUsingXML(element);
    flexibility = Element::getDouble(element->FirstChildElement(MBSIMNS"Flexibility"));
  }

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, ConstantInfluenceFunction, MBSIMNS"ConstantInfluenceFunction")

  void ConstantInfluenceFunction::initializeUsingXML(TiXmlElement *element) {
    InfluenceFunction::initializeUsingXML(element);
    couplingValue = Element::getDouble(element->FirstChildElement(MBSIMNS"CouplingValue"));
  }

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, ConstantFunction<double>, MBSIMNS"ConstantFunction_S")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, ConstantFunction<VecV>, MBSIMNS"ConstantFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, ConstantFunction<Vec3>, MBSIMNS"ConstantFunction_V")

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, LinearFunction<double>, MBSIMNS"LinearFunction_S")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, LinearFunction<VecV>, MBSIMNS"LinearFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, LinearFunction<Vec3>, MBSIMNS"LinearFunction_V")

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, QuadraticFunction<VecV>, MBSIMNS"QuadraticFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, QuadraticFunction<Vec3>, MBSIMNS"QuadraticFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, QuadraticFunction<double>, MBSIMNS"QuadraticFunction_S")

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, SinusFunction<VecV>, MBSIMNS"SinusFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, SinusFunction<Vec3>, MBSIMNS"SinusFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, SinusFunction<double>, MBSIMNS"SinusFunction_S")

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, PositiveFunction<VecV>, MBSIMNS"PositiveFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, PositiveFunction<Vec3>, MBSIMNS"PositiveFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, PositiveFunction<double>, MBSIMNS"PositiveFunction_S")

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, StepFunction<VecV>, MBSIMNS"StepFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, StepFunction<Vec3>, MBSIMNS"StepFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, StepFunction<double>, MBSIMNS"StepFunction_S")

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, TabularFunction<VecV>, MBSIMNS"TabularFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, TabularFunction<Vec3>, MBSIMNS"TabularFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, TabularFunction<double>, MBSIMNS"TabularFunction_S")

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, PeriodicTabularFunction<VecV>, MBSIMNS"PeriodicTabularFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, PeriodicTabularFunction<Vec3>, MBSIMNS"PeriodicTabularFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, PeriodicTabularFunction<double>, MBSIMNS"PeriodicTabularFunction_S")

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, TranslationAlongXAxis<VecV>, MBSIMNS"TranslationAlongXAxis_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, TranslationAlongXAxis<double>, MBSIMNS"TranslationAlongXAxis_S")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, TranslationAlongYAxis<VecV>, MBSIMNS"TranslationAlongYAxis_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, TranslationAlongYAxis<double>, MBSIMNS"TranslationAlongYAxis_S")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, TranslationAlongZAxis<VecV>, MBSIMNS"TranslationAlongZAxis_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, TranslationAlongZAxis<double>, MBSIMNS"TranslationAlongZAxis_S")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, TranslationAlongAxesXY<VecV>, MBSIMNS"TranslationAlongAxesXY_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, TranslationAlongAxesYZ<VecV>, MBSIMNS"TranslationAlongAxesYZ_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, TranslationAlongAxesXZ<VecV>, MBSIMNS"TranslationAlongAxesXZ_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, TranslationAlongAxesXYZ<VecV>, MBSIMNS"TranslationAlongAxesXYZ_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, TranslationAlongFixedAxis<VecV>, MBSIMNS"TranslationAlongFixedAxis_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, TranslationAlongFixedAxis<double>, MBSIMNS"TranslationAlongFixedAxis_S")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, LinearTranslation<VecV>, MBSIMNS"LinearTranslation_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, LinearTranslation<double>, MBSIMNS"LinearTranslation_S")

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, RotationAboutXAxis<VecV>, MBSIMNS"RotationAboutXAxis_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, RotationAboutXAxis<double>, MBSIMNS"RotationAboutXAxis_S")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, RotationAboutYAxis<VecV>, MBSIMNS"RotationAboutYAxis_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, RotationAboutYAxis<double>, MBSIMNS"RotationAboutYAxis_S")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, RotationAboutZAxis<VecV>, MBSIMNS"RotationAboutZAxis_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, RotationAboutZAxis<double>, MBSIMNS"RotationAboutZAxis_S")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, RotationAboutFixedAxis<VecV>, MBSIMNS"RotationAboutFixedAxis_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, RotationAboutFixedAxis<double>, MBSIMNS"RotationAboutFixedAxis_S")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, RotationAboutAxesXY<VecV>, MBSIMNS"RotationAboutAxesXY_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, RotationAboutAxesYZ<VecV>, MBSIMNS"RotationAboutAxesYZ_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, RotationAboutAxesXZ<VecV>, MBSIMNS"RotationAboutAxesXZ_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, RotationAboutAxesXYZ<VecV>, MBSIMNS"RotationAboutAxesXYZ_V")

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, NestedFunction<Vec3(double(VecV))>, MBSIMNS"NestedFunction_VSV")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, NestedFunction<Vec3(double(double))>, MBSIMNS"NestedFunction_VSS")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, NestedFunction<Vec3(VecV(double))>, MBSIMNS"NestedFunction_VVS")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, NestedFunction<Vec3(VecV(VecV))>, MBSIMNS"NestedFunction_VVV")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, NestedFunction<RotMat3(double(VecV))>, MBSIMNS"NestedFunction_MSV")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, NestedFunction<RotMat3(double(double))>, MBSIMNS"NestedFunction_MSS")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, NestedFunction<RotMat3(VecV(double))>, MBSIMNS"NestedFunction_MVS")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, NestedFunction<RotMat3(VecV(VecV))>, MBSIMNS"NestedFunction_MVV")

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, LinearCombinationFunction<VecV>, MBSIMNS"LinearCombinationFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, LinearCombinationFunction<Vec3>, MBSIMNS"LinearCombinationFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, LinearCombinationFunction<double>, MBSIMNS"LinearCombinationFunction_S")

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, VectorValuedFunction<VecV>, MBSIMNS"VectorValuedFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, VectorValuedFunction<Vec3>, MBSIMNS"VectorValuedFunction_V")

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, PiecewiseDefinedFunction<VecV>, MBSIMNS"PiecewiseDefinedFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, PiecewiseDefinedFunction<Vec3>, MBSIMNS"PiecewiseDefinedFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, PiecewiseDefinedFunction<double>, MBSIMNS"PiecewiseDefinedFunction_S")

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, PolynomFunction<VecV>, MBSIMNS"PolynomFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, PolynomFunction<Vec3>, MBSIMNS"PolynomFunction_V")
  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, PolynomFunction<double>, MBSIMNS"PolynomFunction_S")

  MBSIM_OBJECTFACTORY_REGISTERXMLNAME(FunctionBase, TwoDimensionalTabularFunction, MBSIMNS"TwoDimensionalTabularFunction")

  TwoDimensionalTabularFunction::TwoDimensionalTabularFunction() : xVec(Vec(0)), yVec(Vec(0)), XY(Mat(0,0)), xSize(0), ySize(0), x0Index(0), x1Index(0), y0Index(0), y1Index(0), func_value(Vec(1,INIT,0)), xy(Vec(4,INIT,1)), XYval(Vec(4,INIT,0)), XYfac(Mat(4,4,INIT,0)) {
  }

  void TwoDimensionalTabularFunction::calcIndex(const double * x, Vec X, int * xSize, int * xIndexMinus, int * xIndexPlus) {
    if (*x<=X(0)) {
      *xIndexPlus=1;
      *xIndexMinus=0;
      cerr << "TwoDimensionalTabularFunction: Value (" << *x << ") is smaller than the smallest table value(" << X(0) << ")!" << endl;
    }
    else if (*x>=X(*xSize-1)) {
      *xIndexPlus=*xSize-1;
      *xIndexMinus=*xSize-2;
      cerr << "TwoDimensionalTabularFunction: Value (" << *x << ") is greater than the greatest table value(" << X(*xSize-1) << ")!" << endl;
    }
    else {
      if (*x<X(*xIndexPlus))
        while(*x<X(*xIndexPlus-1)&&*xIndexPlus>1)
          (*xIndexPlus)--;
      else if (*x>X(*xIndexPlus))
        while(*x>X(*xIndexPlus)&&*xIndexPlus<*xSize-1)
          (*xIndexPlus)++;
      *xIndexMinus=*xIndexPlus-1;
    }
  }

  void TwoDimensionalTabularFunction::setXValues(Vec xVec_) {
    xVec << xVec_;
    xSize=xVec.size();

    for (int i=1; i<xVec.size(); i++)
      if (xVec(i-1)>=xVec(i))
        throw MBSimError("xVec must be strictly monotonic increasing!");
    xSize=xVec.size();
  }

  void TwoDimensionalTabularFunction::setYValues(Vec yVec_) {
    yVec << yVec_;
    ySize=yVec.size();

    for (int i=1; i<yVec.size(); i++)
      if (yVec(i-1)>=yVec(i))
        throw MBSimError("yVec must be strictly monotonic increasing!");
  }

  void TwoDimensionalTabularFunction::setXYMat(Mat XY_) {
    XY << XY_;

    if(xSize==0)
      cerr << "It is strongly recommended to set x file first! Continuing anyway." << endl;
    else if(ySize==0)
      cerr << "It is strongly recommended to set y file first! Continuing anyway." << endl;
    else {
      if(XY.cols()!=xSize)
        throw MBSimError("Dimension missmatch in xSize");
      else if(XY.rows()!=ySize)
        throw MBSimError("Dimension missmatch in ySize");
    }
  }

  double TwoDimensionalTabularFunction::operator()(const double& x, const double& y) {
    calcIndex(&x, xVec, &xSize, &x0Index, &x1Index);
    calcIndex(&y, yVec, &ySize, &y0Index, &y1Index);

    xy(1)=x;
    xy(2)=y;
    xy(3)=x*y;
    const double x0=xVec(x0Index);
    const double x1=xVec(x1Index);
    const double y0=yVec(y0Index);
    const double y1=yVec(y1Index);
    const double nenner=(x0-x1)*(y0-y1);
    XYval(0)=XY(y0Index, x0Index);
    XYval(1)=XY(y0Index, x1Index);
    XYval(2)=XY(y1Index, x0Index);
    XYval(3)=XY(y1Index, x1Index);
    XYfac(0,0)=x1*y1;
    XYfac(0,1)=-x0*y1;
    XYfac(0,2)=-x1*y0;
    XYfac(0,3)=x0*y0;
    XYfac(1,0)=-y1;   
    XYfac(1,1)=y1;   
    XYfac(1,2)=y0;    
    XYfac(1,3)=-y0;
    XYfac(2,0)=-x1;   
    XYfac(2,1)=x0;    
    XYfac(2,2)=x1;    
    XYfac(2,3)=-x0;
    XYfac(3,0)=1.;    
    XYfac(3,1)=-1.;   
    XYfac(3,2)=-1.;  
    XYfac(3,3)=1.;

    return trans(1./nenner*XYfac*XYval)*xy;
  }

  void TwoDimensionalTabularFunction::initializeUsingXML(TiXmlElement * element) {
    TiXmlElement * e = element->FirstChildElement(MBSIMNS"xValues");
    Vec x_=Element::getVec(e);
    setXValues(x_);
    e = element->FirstChildElement(MBSIMNS"yValues");
    Vec y_=Element::getVec(e);
    setYValues(y_);
    e = element->FirstChildElement(MBSIMNS"xyValues");
    Mat xy_=Element::getMat(e, y_.size(), x_.size());
    setXYMat(xy_);
  }

}