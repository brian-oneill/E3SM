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
#include "AnalysisOperator.h"
#include "AnalysisOpFactory.h"
#include "Config.h"
#include "DataTypes.h"
#include "Dimension.h"
#include "Error.h"
#include "Field.h"
#include "HorzMesh.h"
#include "Logging.h"
#include "TimeMgr.h"
#include "VertCoord.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace OMEGA {

struct OperatorNode {
   std::unique_ptr<AnalysisOperator> Op;       // Operator is owned here
   std::vector<OperatorNode*> Upstream;        // Dependencies
   std::string OutputStreamName;               // Which stream owns it
   Alarm ComputeAlarm;                         // When to compute
};

/// The Analysis class ...
class Analysis {
 public:
   ///
   static void init();

   ///
   static Analysis * create(
       const std::string &Name,
       const HorzMesh *Mesh,
       const VertCoord *VCoord,
       Clock *ModelClock,
       Config *Options);

   ///
   void computeAll();

   ///
   void registerAnalysisOp(const std::string &FieldName,
                           const std::string &OpName,
                           Config &Options);

   ///
   Clock *&getModelClock();

   ///
   const std::vector<OperatorNode> &getOpNodes() const;

   ///
   static Analysis *getDefault();

   ///
   ~Analysis();

   ///
   static void finalize();

 private:
   ///
   static Analysis *DefAnalysis;

   ///
   static std::map<std::string, std::unique_ptr<Analysis>> AllAnalysisObjects;

   ///
   Analysis(const std::string &Name,
            const HorzMesh *Mesh,
            const VertCoord *VCoord,
            Clock *ModelClock,
            Config *Options);

   ///
   std::string Name;

   Clock *ModelClock;
   const HorzMesh *Mesh;
   const VertCoord *VCoord;

   /// All registered operator nodes
   std::vector<OperatorNode> OpNodes;
   /// Full names of registered operators
   std::vector<std::string> RegisteredOpNames;


   // Forbid copy and move construction
   Analysis(const Analysis &) = delete;
   Analysis(Analysis &&)      = delete;

}; // end class Analysis

} // namespace OMEGA
//===----------------------------------------------------------------------===//
#endif // OMEGA_ANALYSIS_H
