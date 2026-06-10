#ifndef OMEGA_ANALYSIS_H
#define OMEGA_ANALYSIS_H

//===-- analysis/Analysis.h - OMEGA Analysis --------------------*- C++ -*-===//
//
/// \file
/// \brief Defines core Analysis framework
///
///
//===----------------------------------------------------------------------===//

#include "operators/Ops.h"
#include "AnalysisGroupManager.h"
#include "AnalysisOperator.h"
#include "AnalysisOpFactory.h"
#include "AnalysisOrchestrator.h"
#include "Config.h"
#include "DataTypes.h"
#include "Dimension.h"
#include "Error.h"
#include "Field.h"
#include "HorzMesh.h"
#include "Logging.h"
#include "TimeMgr.h"
#include "VertCoord.h"

#include <string>
#include <variant>

namespace OMEGA {

class Analysis {
 public:
   static void init();

   static void finalize();
};

} // namespace OMEGA
//===----------------------------------------------------------------------===//
#endif // OMEGA_ANALYSIS_H
