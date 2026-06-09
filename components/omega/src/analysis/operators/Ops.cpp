#include "AnalysisOpFactory.h"
#include "operators/Ops.h"
namespace OMEGA {

void registerAllAnalysisOperators() {
   // Empty! Just being called forces this .o file to be linked
//   std::cout << "Analysis operators module loaded\n";
   AnalysisOpFactory::registerAllArrayVariants<GlobalMaxOp>("global_max");
   AnalysisOpFactory::registerAllArrayVariants<GlobalMinOp>("global_min");
   AnalysisOpFactory::registerAllArrayVariants<GlobalMeanOp>("global_mean");
   AnalysisOpFactory::registerAllArrayVariants<StdDevOp>("standard_dev");
   AnalysisOpFactory::registerAllArrayVariants<TimeMeanOp>("time_mean");
}

}
