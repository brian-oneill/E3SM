#include "AnalysisOpFactory.h"
#include "analysisOperators/Ops.h"
#include <iostream>
namespace OMEGA {
//template class GlobalMaxOp<R8>;
using GlobalMaxOp2DR8 = GlobalMaxOp<Array2DR8>;

void registerAllAnalysisOperators() {
   // Empty! Just being called forces this .o file to be linked
//   std::cout << "Analysis operators module loaded\n";
   AnalysisOpFactory::registerAllArrayVariants<GlobalMaxOp>("global_max");
}

//REGISTER_DIAG_OPERATOR(GlobalMaxOp2DR8, "global_max_Array2DR8");

}
