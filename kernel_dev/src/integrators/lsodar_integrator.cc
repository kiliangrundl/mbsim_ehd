/* Copyright (C) 2004-2006  Martin Förg
 
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
 * Contact:
 *   mfoerg@users.berlios.de
 *
 */

#include <config.h>
#include <fmatvec.h>
#include "multi_body_system.h"
#include "fortran_wrapper.h"
#include "lsodar_integrator.h"
#include <fstream>

#ifndef NO_ISO_14882
using namespace std;
#endif

using namespace fmatvec;

namespace MBSim {

  LSODARIntegrator::LSODARIntegrator() : dtMax(0), dtMin(0), aTol(1,INIT,1e-6), rTol(1e-6), dt0(0) {
  }

  void LSODARIntegrator::fzdot(int* zSize, double* t, double* z_, double* zd_) {
    Vec z(*zSize, z_);
    Vec zd(*zSize, zd_);
    system->zdot(z, zd, *t);
  }

  void LSODARIntegrator::fsv(int* zSize, double* t, double* z_, int* nsv, double* sv_) {
    Vec z(*zSize, z_);
    Vec sv(*nsv, sv_);
    system->getsv(z, sv, *t);
  }

  void LSODARIntegrator::integrate(MultiBodySystem& system_) {
    system = &system_;

    int zSize=system->getzSize();
    Vec z(zSize);
    if(z0.size())
      z = z0;
    else
      system->initz(z);
    system->computeInitialCondition();
    double t=0.0;
    double tPlot=t+dtPlot;

    int iTol; 
    if(aTol.size() == 1) {
      iTol = 1; // Skalar
    } else {
      iTol = 2; // Vektor
      assert (aTol.size() >= zSize);
    }

//    int i, one=1, zero=0, two=2, istate=1;
    int one=1, two=2, istate=1;

    int nsv=system->getsvSize();
    int lrWork = (22+zSize*max(16,zSize+9)+3*nsv)*2;
    Vec rWork(lrWork);
    dt0 = 1e-13;
    rWork(4) = dt0; // integrator chooses the step size (dont use dt0)
    rWork(5) = 1e-2;
    rWork(6) = 1e-14;
    int liWork=(20+zSize)                             *10;//////////////;
    Vector<int> iWork(liWork);
    iWork(5) = 10000;

    system->plot(z, t);

    double s0 = clock();
    double time = 0;
    int integrationSteps = 0;

    ofstream integPlot((system->getDirectoryName() + name + ".plt").c_str());

    Vector<int> jsv(nsv);  
    bool donedrift;

    cout.setf(ios::scientific, ios::floatfield);

    while(t<tEnd) {  
      
      integrationSteps++;

      DLSODAR(fzdot, &zSize, z(), &t, &tPlot, &iTol, &rTol, aTol(), &one,
	  &istate, &one, rWork(), &lrWork, iWork(),
	  &liWork, NULL, &two, fsv, &nsv, jsv());
      if(istate==2 || t==tPlot) {
	system->plot(z, t);
	if(output)
	  cout << "   t = " <<  t << ",\tdt = "<< rWork(10) << "\r"<<flush;
	double s1 = clock();
	time += (s1-s0)/CLOCKS_PER_SEC;
	s0 = s1; 
	integPlot<< t << " " << rWork(10) << " " << time << endl;
	tPlot += dtPlot;
	donedrift=system->driftCompensation(z, t);
	if(donedrift==true)
	  istate=1;
      }
      if(istate==3) {
	system->shift(z, jsv, t);
	system->plot(z, t);
	istate=1;
	rWork(4)=dt0;
      }
      if(istate<0) exit(istate);
    }

    integPlot.close();

    ofstream integSum((system->getDirectoryName() + name + ".sum").c_str());
    integSum << "Integration time: " << time << endl;
    integSum << "Integration steps: " << integrationSteps << endl;
    integSum.close();

    cout.unsetf (ios::scientific);
    cout << endl;
  }

}
