#ifndef _SYSTEM_H
#define _SYSTEM_H

#include "mbsim/dynamic_system_solver.h"
#include "mbsimFlexibleBody/flexible_body/flexible_body_linear_external_ffr.h"
#include "mbsim/rigid_body.h"
#include <string>

class SliderCrankSystem : public MBSim::DynamicSystemSolver {
  public:
    SliderCrankSystem(const std::string &projectName);

};

#endif /* _SYSTEM_H */

