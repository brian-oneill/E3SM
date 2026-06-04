#include "AnalysisOpFactory.h"
#include "analysisOperators/Ops.h"
#include <iostream>
namespace OMEGA {
//template class GlobalMaxOp<R8>;
using GlobalMaxOpR8 = GlobalMaxOp<R8>;

void registerAllAnalysisOperators() {
   // Empty! Just being called forces this .o file to be linked
   std::cout << "Analysis operators module loaded\n";
}

REGISTER_DIAG_OPERATOR(GlobalMaxOpR8, "global_max_R8");

}
