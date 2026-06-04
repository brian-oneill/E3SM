#include "AnalysisOpFactory.h"
#include "analysisOperators/Ops.h"
namespace OMEGA {
//template class GlobalMaxOp<R8>;
using GlobalMaxOpR8 = GlobalMaxOp<R8>;
REGISTER_DIAG_OPERATOR(GlobalMaxOpR8, "global_max_R8");
}
