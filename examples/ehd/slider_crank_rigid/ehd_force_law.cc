/* Copyright (C) 2004-2015 MBSim Development Team
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
 * Contact: martin.o.foerg@googlemail.com
 */

#include "ehd_force_law.h"
#include "circlesolid_circlehollow_ehd.h"

#include <mbsim/single_contact.h>

using namespace MBSim;
using namespace fmatvec;
using namespace xercesc;
using namespace MBXMLUtils;

EHDForceLaw::EHDForceLaw() {

}

EHDForceLaw::~EHDForceLaw() {
}

void EHDForceLaw::computeSmoothForces(std::vector<std::vector<SingleContact> > & contacts) {
// Here the force computation for all contact pairings happens
  for (std::vector<std::vector<SingleContact> >::iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
    for (std::vector<SingleContact>::iterator jter = iter->begin(); jter != iter->end(); ++jter) {
      (*jter).getlaN()(0) = 1e7*nrm2((static_cast<ContactKinematicsCircleSolidCircleHollowEHD*>(jter->getContactKinematics()))->getWrD());
    }
  }
}

void EHDForceLaw::initializeUsingXML(xercesc::DOMElement* element) {
  xercesc::DOMElement *e;
  //TODO ...

}
