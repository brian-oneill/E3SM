//===-- analysis/Analysis.cpp - analysis module -----------------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//

#include "Analysis.h"
#include "analysisGroups/Groups.h"
#include "IOStream.h"
#include "TimeStepper.h"
 
namespace OMEGA {

// create the static class members
Analysis *Analysis::DefAnalysis = nullptr;
std::map<std::string, std::unique_ptr<Analysis>> Analysis::AllAnalysisObjects;


//------------------------------------------------------------------------------
void Analysis::init() {

   registerAllBaseAnalysisOperators();

   auto Mesh = HorzMesh::getDefault();

   auto VCoord = VertCoord::getDefault();

   auto DefTimeStepper = TimeStepper::getDefault();

   Clock *OmegaClock = DefTimeStepper->getClock();

   Config *OmegaConfig = Config::getOmegaConfig();

   Analysis::DefAnalysis =
       create("Default", Mesh, VCoord, OmegaClock, OmegaConfig);

//   IOStream::printAllStreams();
} // end init

//------------------------------------------------------------------------------
Analysis *Analysis::create(const std::string &Name,
                        const HorzMesh *Mesh,
                        const VertCoord *VCoord,
                        Clock *ModelClock,
                        Config *Options) {

   if (AllAnalysisObjects.find(Name) != AllAnalysisObjects.end()) {
      LOG_ERROR("Attempted to create an Analysis with name {} "
                "but one with that name already exists", Name);
      return nullptr;
   }

   auto NewAnalysis = new Analysis(Name, Mesh, VCoord, ModelClock, Options);

   AllAnalysisObjects.emplace(Name, NewAnalysis);

   return NewAnalysis;

} // end create


//------------------------------------------------------------------------------
Analysis::Analysis(const std::string &InName,
                        const HorzMesh *InMesh,
                        const VertCoord *InVCoord,
                        Clock *InModelClock,
                        Config *Options) {

   Name = InName;
   Mesh = InMesh;
   VCoord = InVCoord;
   ModelClock = InModelClock;

   std::string NamePrefix = Name + "_";
   if (Name == "Default") {
      NamePrefix = "";
   }

   Error Err;
   Config AnalysisCfg("Analysis");
   Err += Options->get(AnalysisCfg);
   CHECK_ERROR_ABORT(Err, "Analysis: Analysis group not in Config");

   for (Config::Iter It = AnalysisCfg.begin(); It != AnalysisCfg.end(); ++It) {
      std::string GroupName;
      AnalysisCfg.getName(It, GroupName);
      Config GroupCfg(GroupName);
      Err += AnalysisCfg.get(GroupCfg);
      bool GroupEnabled = false;
      GroupCfg.get("Enable", GroupEnabled);
      if (GroupEnabled) {
         std::cout << GroupName << " " << GroupEnabled << std::endl;
         if (GroupName == "GlobalStats") {
            GlobalStats GlobalStatsGroup(NamePrefix + GroupName, GroupCfg, this);

            continue;
         }
      }
   } 

} // end constructor

//------------------------------------------------------------------------------
void Analysis::registerAnalysisOp(
    const std::string &OpName,
    const std::vector<std::string> &UpstreamNames,
    Config &Options) {

   auto NewOp = AnalysisOpFactory::createOp(OpName, UpstreamNames, Options);

//   std::cout << "register op w name:  " << NewOp->getName() << "| exists: " << OpNodeExists(NewOp->getName()) << std::endl;

   if (NewOp and !OpNodeExists(NewOp->getName())) {

      OperatorNode Node;
      Node.Op = std::move(NewOp);

      Node.Upstream = {nullptr};
      Node.StreamName = "";
      Alarm NewAlarm;
      Node.ComputeAlarm = NewAlarm;

      OpNodes.push_back(std::move(Node));

   }

} // end registerAnalysisOp

//------------------------------------------------------------------------------
Clock *&Analysis::getModelClock(){
   return ModelClock;
}

//------------------------------------------------------------------------------
const std::vector<OperatorNode> &Analysis::getOpNodes() const {return OpNodes;}

//------------------------------------------------------------------------------
bool Analysis::OpNodeExists(const std::string &FullOpName) {

   for (const auto &OpNode: OpNodes) {
      if (FullOpName == OpNode.Op->getName()) {
         return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------
Analysis *Analysis::getDefault() {return DefAnalysis;}

//------------------------------------------------------------------------------
void Analysis::finalize() {

   AllAnalysisObjects.clear();

}

Analysis::~Analysis() {}


} // end namespace OMEGA


//===----------------------------------------------------------------------===//
