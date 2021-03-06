#include "system.h"
#include <mbsim/integrators/integrators.h>

using namespace MBSim;
using namespace MBSimIntegrator;
using namespace std;

int main (int argc, char* argv[]) {

  System *sys = new System("MBS");

  sys->setStopIfNoConvergence(true,true);
  sys->initialize();

  TimeSteppingIntegrator integrator;
//  RADAU5Integrator integrator;
  integrator.setEndTime(1.);
  integrator.setStepSize(1e-3);
  integrator.setPlotStepSize(1e-2);

  integrator.integrate(*sys);

  sys->closePlot();

  cout << "finished"<<endl;

  delete sys;

  return 0;
}

