#include "slider_crank_system.h"
#include <mbsim/integrators/integrators.h>

#include "SimJournalBearing.h"

using namespace MBSim;
using namespace MBSimIntegrator;
using namespace std;

int main(int argc, char* argv[]) {

  if (1) {
    // Testing code with the SimJournabBearing-file
    SimJournalBearing();
  }
  else {
    // The following code follows the standard structure of MBSim (this it how the example should be like in the end)

    // create the system: Here the complete defintion is done
    SliderCrankSystem *sys = new SliderCrankSystem("MBS");

    // just some solver settings for the solution of the non-smooth contact/impact laws
//  sys->setStopIfNoConvergence(true,true);

// initialize the system -> step after the definition of the single relation between bodies/links/frames etc.
    sys->initialize();

    /* Integrator settings*/

//  TimeSteppingIntegrator integrator;
//  integrator.setStepSize(1e-5);

    RADAU5Integrator integrator;
    integrator.setEndTime(2e-2);
    integrator.setPlotStepSize(1e-4);

    // integrate the system
    integrator.integrate(*sys);

//  sys->closePlot();

    cout << "finished" << endl;

    delete sys;
  }

  return 0;
}

