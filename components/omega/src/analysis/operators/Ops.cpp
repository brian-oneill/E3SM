//===----------------------------------------------------------------------===//
#include "Analysis.h"
#include "AnalysisOpFactory.h"
#include "operators/Ops.h"
namespace OMEGA {

//------------------------------------------------------------------------------
void Analysis::registerAllBaseAnalysisOperators() {
//   std::cout << "Analysis operators module loaded\n";
   AnalysisOpFactory::registerAllArrayVariants<SpatialMaxOp>("SpatialMax");
   AnalysisOpFactory::registerAllArrayVariants<SpatialMinOp>("SpatialMin");
   AnalysisOpFactory::registerAllArrayVariants<SpatialMeanOp>("SpatialMean");
   AnalysisOpFactory::registerAllArrayVariants<SpatialStdDevOp>("SpatialStdDev");
   AnalysisOpFactory::registerAllArrayVariants<TimeMeanOp>("TimeMean");
}

} // end namespace OMEGA
