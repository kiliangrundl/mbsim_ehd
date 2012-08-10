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
#include "mbsim/tree.h"
#include "mbsim/object.h"
#include "mbsim/link.h"
#include "mbsim/frame.h"
#include "mbsim/extra_dynamic.h"

using namespace fmatvec; 
using namespace std;

namespace MBSim {

  void Node::addChild(Node* child_) {
    child.push_back(child_);
  }

  void Node::updateStateDependentVariables(double t) {
    obj->updateStateDependentVariables(t);
    for(unsigned int i=0; i<child.size(); i++)
      child[i]->updateStateDependentVariables(t);
  }

  void Node::updateJacobians(double t) {
    obj->updateJacobians(t);
    for(unsigned int i=0; i<child.size(); i++)
      child[i]->updateJacobians(t);
  }

  void Node::updateInverseKineticsJacobians(double t) {
    obj->updateInverseKineticsJacobians(t);
    for(unsigned int i=0; i<child.size(); i++)
      child[i]->updateInverseKineticsJacobians(t);
  }

  void Node::calcqSize(int &qSize) {
    obj->calcqSize();
    obj->setqInd(qSize);
    qSize += obj->getqSize();

    for(unsigned int i=0; i<child.size(); i++)
      child[i]->calcqSize(qSize);
  }

  void Node::calcuSize(int &uSize, int j) {

    obj->calcuSize(j);
    obj->setuInd(uSize,j);
    uSize += obj->getuSize(j);

    for(unsigned int i=0; i<child.size(); i++)
      child[i]->calcuSize(uSize,j);
  }

  void Node::sethSize(int &hSize, int j) {

    for(int i=child.size()-1; i>=0; i--)
      child[i]->sethSize(hSize,j);

    obj->sethSize(hSize,j);
    obj->sethInd(0,j);
    hSize -= obj->getuSize(j);
  }


  Tree::Tree(const string &projectName) : DynamicSystem(projectName) {}

  Tree::~Tree() {}

  void Tree::updateStateDependentVariables(double t) {
    root->updateStateDependentVariables(t);
  }

  void Tree::updateJacobians(double t) {
    root->updateJacobians(t);

    for(vector<Link*>::iterator i = link.begin(); i != link.end(); ++i) 
      (*i)->updateJacobians(t);
  }

  void Tree::updatedu(double t, double dt) {
    ud = slvLLFac(LLM, h*dt+r);
  }

  void Tree::updatezd(double t) {
    qd = T*u;
    ud =  slvLLFac(LLM, h+r);

    for(vector<DynamicSystem*>::iterator i = dynamicsystem.begin(); i != dynamicsystem.end(); ++i) 
      (*i)->updatexd(t);

    for(vector<Link*>::iterator i = link.begin(); i != link.end(); ++i)
      (**i).updatexd(t);

    for(vector<ExtraDynamic*>::iterator i = extraDynamic.begin(); i!= extraDynamic.end(); ++i) 
      (**i).updatexd(t);
  }

  void Tree::sethSize(int hSize_, int j) {
    hSize[j] = hSize_;
    root->sethSize(hSize_,j);
  } 

  void Tree::calcqSize() {
    qSize = 0;
    root->calcqSize(qSize);
  }

  void Tree::calcuSize(int j) {
    uSize[j] = 0;
    root->calcuSize(uSize[j],j);
  }

  void Tree::updateInverseKineticsJacobians(double t) {
    root->updateInverseKineticsJacobians(t);

    for(vector<Link*>::iterator i = link.begin(); i != link.end(); ++i) 
      (*i)->updateJacobians(t);
  }

  void Tree::facLLM() {
    LLM = facLL(M); 
  }

  Node* Tree::addObject(Node* tree, Object* obj) {
    DynamicSystem::addObject(obj);

    Node *node = new Node(obj);
    if(tree)
      tree->addChild(node);
    else
      root = node;
    return node;
  }

  Node* Tree::addTree(Node* tree, Tree *sys) {
    DynamicSystem::addGroup(sys);

    Node *node = new Node(sys);
    if(tree)
      tree->addChild(node);
    else
      root = node;
    return node;
  }

}
