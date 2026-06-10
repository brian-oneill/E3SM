#ifndef OMEGA_OPS_H
#define OMEGA_OPS_H

#include "operators/SpatialMaxOp.h"
#include "operators/SpatialMeanOp.h"
#include "operators/SpatialMinOp.h"
#include "operators/StdDevOp.h"
#include "operators/TimeMeanOp.h"

namespace OMEGA {

void registerAllAnalysisOperators();

}
#endif
