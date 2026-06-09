#ifndef OMEGA_OPS_H
#define OMEGA_OPS_H

#include "operators/GlobalMaxOp.h"
#include "operators/GlobalMeanOp.h"
#include "operators/GlobalMinOp.h"
#include "operators/StdDevOp.h"
#include "operators/TimeMeanOp.h"

namespace OMEGA {

void registerAllAnalysisOperators();

}
#endif
