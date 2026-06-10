#include "AnalysisOrchestrator.h"
#include "analysisGroups/Groups.h"
#include "IOStream.h"
#include "TimeStepper.h"


namespace OMEGA {

AnalysisOrchestrator *AnalysisOrchestrator::DefOrchestrator = nullptr;
std::map<std::string, std::unique_ptr<AnalysisOrchestrator>>
   AnalysisOrchestrator::AllOrchestrators;

void AnalysisOrchestrator::init() {

   auto Mesh = HorzMesh::getDefault();
   auto VCoord = VertCoord::getDefault();

   auto DefTimeStepper = TimeStepper::getDefault();

   Clock *OmegaClock     = DefTimeStepper->getClock();

   Config *OmegaConfig = Config::getOmegaConfig();

   AnalysisOrchestrator::DefOrchestrator = create("Default", Mesh, VCoord, OmegaClock, OmegaConfig);

}

AnalysisOrchestrator *AnalysisOrchestrator::create(const std::string &Name,
                        const HorzMesh *Mesh,
                        const VertCoord *VCoord,
                        Clock *ModelClock,
                        Config *Options) {

   if (AllOrchestrators.find(Name) != AllOrchestrators.end()) {
      LOG_ERROR("Attempted to create an AnalysisOrchestrator with name {} "
                "but one with that name already exists", Name);
      return nullptr;
   }

   auto NewOrchestrator = new AnalysisOrchestrator(Name, Mesh, VCoord, ModelClock, Options);
   AllOrchestrators.emplace(Name, NewOrchestrator);

   return NewOrchestrator;

}

AnalysisOrchestrator::AnalysisOrchestrator(const std::string &InName,
                        const HorzMesh *InMesh,
                        const VertCoord *InVCoord,
                        Clock *ModelClock,
                        Config *Options) {

   Name = InName;

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
            GlobalStats GlobalStatsGroup(Name, GroupCfg, this);



            continue;
         }
      }
   } 

}

void AnalysisOrchestrator::registerAnalysisOp(std::string &FieldName,
                                         std::string &OpName,
                                         Config &Options) {

   auto NewOp = AnalysisOpFactory::createOp(OpName, FieldName, Options);

   if (NewOp) {

      OperatorNode Node;
      Node.Op = std::move(NewOp);

   }

}

void AnalysisOrchestrator::clear() {
   AllOrchestrators.clear();
}

AnalysisOrchestrator::~AnalysisOrchestrator() {}

} // end namespace OMEGA
