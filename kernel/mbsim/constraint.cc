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

#include <config.h>
#include "constraint.h"
#include "rigid_body.h"
#include "utils/nonlinear_algebra.h"

using namespace MBSim;
using namespace fmatvec;
using namespace std;

class Residuum : public Function1<Vec,Vec> {
  RigidBody *body1, *body2;
  Mat d;
  Frame *frame1, *frame2;
  double t;
  int i1,i2;
  public:
    Residuum(RigidBody* body1_, RigidBody* body2_, const Mat &d_,Frame *frame1_, Frame *frame2_,double t_,int i1_, int i2_) : body1(body1_),body2(body2_),d(d_),frame1(frame1_), frame2(frame2_), t(t_), i1(i1_), i2(i2_) {}
    Vec operator()(const Vec &x, const void * =NULL) {
      Vec res(2); 

      body1->getqRel()(0) = x(0);
      body2->getqRel()(0) = x(1);

      body1->updatePositionAndOrientationOfFrame(t,i1);
      body2->updatePositionAndOrientationOfFrame(t,i2);

      res = d.T()*(frame1->getPosition()-frame2->getPosition()); 

      return res;
    } 
};

Constraint::Constraint(const std::string &name) : Object(name) {
}

Constraint1::Constraint1(const std::string &name, RigidBody *b0, RigidBody* b1, RigidBody* b2, Frame* frame1_, Frame* frame2_) : Constraint(name), frame1(frame1_), frame2(frame2_) {
  bi = b0;
  bd1 = b1;
  bd2 = b2;
  bd1->addDependency(this);
  bd2->addDependency(this);
}

void Constraint1::init(InitStage stage) {
  if(stage==preInit) {
    Constraint::init(stage);
    dependency.push_back(bi);
  }
  else
    Constraint::init(stage);
}

void Constraint1::updateStateDependentVariables(double t){
  Mat d("[1,0;0,1;0,0]");
  Residuum* f = new Residuum(bd1,bd2,d,frame1,frame2,t,2,0);
  MultiDimNewtonMethod newton(f);
  Vec x0(2);
  x0(0) = bd1->getqRel()(0);
  x0(1) = bd2->getqRel()(0);
  Vec q12 = newton.solve(x0);
  bd1->getqRel()(0) = q12(0);
  bd2->getqRel()(0) = q12(1);

  bd1->updateVelocities(t,2);
  bd2->updateVelocities(t,0);
  Mat J12(3,2);
  J12(Index(0,2),Index(0,0)) = bd1->getWJTrel();
  J12(Index(0,2),Index(1,1)) = bd2->getWJTrel();
  Vec u12 = slvLU(SqrMat(d.T()*J12),-d.T()*(bd1->getWjTrel()-bd2->getWjTrel())); 
  bd1->getuRel()(0) = u12(0);
  bd2->getuRel()(0) = u12(1);
}

void Constraint1::updateJacobians(double t){
  Mat d("[1,0;0,1;0,0]");
  bd1->updateAcclerations(t,2);
  bd2->updateAcclerations(t,0);

  Mat J12t(3,2);
  J12t(Index(0,2),Index(0,0)) = bd1->getWJTrel();
  J12t(Index(0,2),Index(1,1)) = bd2->getWJTrel();

  Mat J12 = slvLU(SqrMat(d.T()*J12t),-d.T()*(frame1->getJacobianOfTranslation()-frame2->getJacobianOfTranslation())); 
  Vec j12 = slvLU(SqrMat(d.T()*J12t),-d.T()*(frame1->getGyroscopicAccelerationOfTranslation()-frame2->getGyroscopicAccelerationOfTranslation()));

  bd1->getJRel()(0,0) = J12(0,0);
  bd2->getJRel()(0,0) = J12(1,0);
  bd1->getjRel()(0) = j12(0);
  bd2->getjRel()(0) = j12(1);

}

Constraint2::Constraint2(const std::string &name, RigidBody* body) : Constraint(name), bd(body) {
  bd->addDependency(this);
}

void Constraint2::init(InitStage stage) {
  if(stage==preInit) {
    Constraint::init(stage);
    for(unsigned int i=0; i<bi.size(); i++)
      dependency.push_back(bi[i]);
  }
  else
    Constraint::init(stage);
}

void Constraint2::addDependency(RigidBody* body, double ratio_) {
  bi.push_back(body); 
  ratio.push_back(ratio_);
}

void Constraint2::updateStateDependentVariables(double t){
  bd->getqRel().init(0);
  bd->getuRel().init(0);
  for(unsigned int i=0; i<bi.size(); i++) {
    bd->getqRel() += bi[i]->getqRel()*ratio[i];
    bd->getuRel() += bi[i]->getuRel()*ratio[i];
  }
}

void Constraint2::updateJacobians(double t){
  bd->getJRel().init(0); 
  for(unsigned int i=0; i<bi.size(); i++) {
    bd->getJRel()(Index(0,bi[i]->getJRel().rows()-1),Index(0,bi[i]->getJRel().cols()-1)) += bi[i]->getJRel()*ratio[i];
  }
}