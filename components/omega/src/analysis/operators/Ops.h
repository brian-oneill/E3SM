#ifndef OMEGA_OPS_H
#define OMEGA_OPS_H

//===----------------------------------------------------------------------===//

#include "operators/SpatialMaxOp.h"
#include "operators/SpatialMeanOp.h"
#include "operators/SpatialMinOp.h"
#include "operators/SpatialStdDevOp.h"
#include "operators/TimeMeanOp.h"

namespace OMEGA {

///
void registerAllBaseAnalysisOperators();

}  // end namespace OMEGA
#endif
