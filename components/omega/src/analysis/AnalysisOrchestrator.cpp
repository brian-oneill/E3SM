#include "AnalysisOrchestrator.h"
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
                        const Config *Options) {

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
                        const Config *Options) {

}

void AnalysisOrchestrator::clear() {
   AllOrchestrators.clear();
}

} // end namespace OMEGA
