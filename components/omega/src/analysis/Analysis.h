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
#include "MachEnv.h"
#include "TimeMgr.h"
#include "VertCoord.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace OMEGA {

///
std::vector<std::string> parseFreqStr(const std::string &FreqStr);

/// The OperatorNode struct ...
struct OperatorNode {
   std::unique_ptr<AnalysisOperator> Op;       // Operator is owned here
   std::vector<OperatorNode*> Upstreams;       // Upstream dependencies
   std::vector<std::string> StreamName;        // Which stream owns it
   std::vector<Alarm*> ComputeAlarms;          // Pointers to alarms (owned by streams or Analysis)
};

/// The AnalysisStream struct ...
struct AnalysisStream {
   // AnalysisStream constructor
   AnalysisStream(std::string InStreamName, TimeInterval InPeriodInterval, bool InIsTimeReduction)
   : StreamName(InStreamName), PeriodInterval(InPeriodInterval), IsTimeReduction(InIsTimeReduction) {}
   std::string StreamName;      // Name of stream
   TimeInterval PeriodInterval; // TimeInterval form of interval
   bool IsTimeReduction;              // Is temporal average or discrete samples
};


/// The Analysis class ...
class Analysis {
 public:
   /// Analysis output stream info
   std::vector<AnalysisStream> OutputStreams;

   ///
   static void init();

   ///
   static Analysis * create(
       const std::string &Name,
       const MachEnv *Env,
       const HorzMesh *Mesh,
       const VertCoord *VCoord,
       Clock *ModelClock,
       Config *Options
   );

   ///
   void computeAll();

   ///
   void parseChainAndBuildOps(
      const std::string &OpChainStr
   );


   ///
   void registerAnalysisOp(
       const std::string &OpName,
       const std::vector<std::string> &UpstreamNames,
       Config Options
   );

   ///
   Clock *&getModelClock();

   ///
   const std::vector<OperatorNode*> getOpNodes();

   ///
   bool OpNodeExists(const std::string &FullOpName);

   ///
   static Analysis *getDefault();

   ///
   ~Analysis();

   ///
   static void finalize();

 private:

   // Storage for accumulation alarms (owned by Analysis, not streams)
   std::vector<std::unique_ptr<Alarm>> AccumulationAlarms;

   ///
   static Analysis *DefAnalysis;

   ///
   static std::map<std::string, std::unique_ptr<Analysis>> AllAnalysisObjects;

   ///
   Analysis(const std::string &Name,
            const MachEnv *Env,
            const HorzMesh *Mesh,
            const VertCoord *VCoord,
            Clock *ModelClock,
            Config *Options
   );

   ///
   std::string Name;
   const MachEnv *Env;
   const HorzMesh *Mesh;
   const VertCoord *VCoord;
   Clock *ModelClock;

   /// All registered operator nodes
   std::vector<OperatorNode> OpNodes;
   /// Full names of registered operators
   std::vector<std::string> RegisteredOpNames;

   ///
   void buildOperatorDependencies();

   ///
   void setComputeAlarms();

   ///
   void initializeAllOps();

   /// 
   TimeInterval findShortestStreamInterval(const OperatorNode &Node);

   ///
   TimeInterval findShortestDownstreamAlarm(const OperatorNode &Node);

   ///
   void propagateAlarmsUpstream();
   
   // Forbid copy and move construction
   Analysis(const Analysis &) = delete;
   Analysis(Analysis &&)      = delete;

}; // end class Analysis

} // namespace OMEGA
//===----------------------------------------------------------------------===//
#endif // OMEGA_ANALYSIS_H
