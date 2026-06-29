//===-- analysis/operators/Ops.cpp - Operator registration ------*- C++ -*-===//
//
// Implementation of the Analysis method that registers all base analysis
// operators with the AnalysisOpFactory. This registration occurs during
// Analysis initialization, making all operators available for runtime
// instantiation via the factory. Each registerAllArrayVariants() call
// registers dozens of type-specific variants for a single operator template,
// covering all combinations of scalar type, rank, and memory location.
//
//===----------------------------------------------------------------------===//

#include "Analysis.h"
#include "AnalysisOpFactory.h"
#include "operators/Ops.h"

namespace OMEGA {

//------------------------------------------------------------------------------
// Registers all base analysis operators with the AnalysisOpFactory. Called
// during Analysis initialization to populate the factory registry. Each
// registration call expands a single operator template over all supported
// array type combinations (scalar type, rank, memory location), enabling
// type-safe dispatch at operator creation time.
void Analysis::registerAllBaseAnalysisOperators() {
   
   AnalysisOpFactory::registerAllArrayVariants<SpatialMaxOp>("SpatialMax");
   AnalysisOpFactory::registerAllArrayVariants<SpatialMinOp>("SpatialMin");
   AnalysisOpFactory::registerAllArrayVariants<SpatialMeanOp>("SpatialMean");
   AnalysisOpFactory::registerAllArrayVariants<SpatialStdDevOp>("SpatialStdDev");
   AnalysisOpFactory::registerAllArrayVariants<TimeMeanOp>("TimeMean");
   
} // end registerAllBaseAnalysisOperators

} // end namespace OMEGA

//===----------------------------------------------------------------------===//
