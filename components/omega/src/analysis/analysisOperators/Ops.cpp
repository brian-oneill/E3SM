#include "AnalysisOpFactory.h"
#include "analysisOperators/Ops.h"
#include <iostream>
namespace OMEGA {
//template class GlobalMaxOp<R8>;
using GlobalMaxOp1DR8 = GlobalMaxOp<Array1DR8>;

void registerAllAnalysisOperators() {
   // Empty! Just being called forces this .o file to be linked
   std::cout << "Analysis operators module loaded\n";
}

REGISTER_DIAG_OPERATOR(GlobalMaxOp1DR8, "global_max_1DR8");

}
