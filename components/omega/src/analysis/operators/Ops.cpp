#include "AnalysisOpFactory.h"
#include "operators/Ops.h"
namespace OMEGA {

void registerAllAnalysisOperators() {
//   std::cout << "Analysis operators module loaded\n";
   AnalysisOpFactory::registerAllArrayVariants<SpatialMaxOp>("spatial_max");
   AnalysisOpFactory::registerAllArrayVariants<SpatialMinOp>("spatial_min");
   AnalysisOpFactory::registerAllArrayVariants<SpatialMeanOp>("spatial_mean");
   AnalysisOpFactory::registerAllArrayVariants<StdDevOp>("standard_dev");
   AnalysisOpFactory::registerAllArrayVariants<TimeMeanOp>("time_mean");
}

}
