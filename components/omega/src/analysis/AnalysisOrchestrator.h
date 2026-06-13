#ifndef OMEGA_ANALYSISORCHESTRATOR_H
#define OMEGA_ANALYSISORCHESTRATOR_H

#include "AnalysisOperator.h"
#include "AnalysisOpFactory.h"
#include "Config.h"
#include "HorzMesh.h"
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

class AnalysisOrchestrator {
 public:
   static void init();

   void computeAll();

   static void clear();

   static AnalysisOrchestrator * create(const std::string &Name,
                        const HorzMesh *Mesh,
                        const VertCoord *VCoord,
                        Clock *ModelClock,
                        Config *Options);

   static AnalysisOrchestrator *getDefault();

   ~AnalysisOrchestrator();

   void registerAnalysisOp(const std::string &FieldName,
                           const std::string &OpName,
                           Config &Options);

   Clock *&getModelClock();

   const std::vector<OperatorNode> &getOpNodes() const;

 private:

   AnalysisOrchestrator(const std::string &Name,
                        const HorzMesh *Mesh,
                        const VertCoord *VCoord,
                        Clock *ModelClock,
                        Config *Options);

   std::string Name;

   Clock *ModelClock;
   const HorzMesh *Mesh;
   const VertCoord *VCoord;


   /// All registered operators
   std::vector<OperatorNode> OpNodes;

   static AnalysisOrchestrator *DefOrchestrator;
   static std::map<std::string, std::unique_ptr<AnalysisOrchestrator>> AllOrchestrators;

   // Forbid copy and move construction
   AnalysisOrchestrator(const AnalysisOrchestrator &) = delete;
   AnalysisOrchestrator(AnalysisOrchestrator &&)      = delete;


};

} // end namespace OMEGA

#endif
