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
// Local utility routine that breaks a string into a numeric substring and a
// character substring, assuming the numeric substring is first.
std::vector<std::string> parseFreqStr(const std::string &FreqStr) {

   std::string DigitStr;
   std::string UnitsStr;
   size_t Pos = FreqStr.find_first_not_of("0123456789");
   if (Pos != std::string::npos) {
      DigitStr = FreqStr.substr(0, Pos);
      UnitsStr = FreqStr.substr(Pos);
   }
   if (FreqStr == "" or UnitsStr == "") {
      ABORT_ERROR("Analysis: Invalid frequency string found in Config: {}", FreqStr);
   }
   if (UnitsStr.back() != 's') {
      UnitsStr += 's';
   }

   return {DigitStr, UnitsStr};

}

//------------------------------------------------------------------------------
// Initialize the Analysis module by creating the default Analysis instance, and
// registering all operator types for the AnalysisOpFactor requires prior
// initialization of the HorzMesh, VertCoord, and TimeStepper
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
// Creates a new Analysis instance using the constructor and puts it in the
// AllAnalysisObjects map.
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
// Construct a new Analysis instance. Loops over the contents of the Analysis
// node in the config file, calls derived constructor for pre-defined
// AnalysisGroups, or parses composable operator chains for custom groups.
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
//         std::cout << GroupName << " " << GroupEnabled << std::endl;
         if (GroupName == "GlobalStats") {
            GlobalStats GlobalStatsGroup(NamePrefix + GroupName, GroupCfg, this);

            continue;
         }
         ABORT_ERROR("Analysis: custom analysis group enabled in config, but"
                     "composable operators are not yet supported.");
      }
   } 

   buildOperatorDependencies();

   setComputeAlarms();
} // end constructor

//------------------------------------------------------------------------------
// For a given string defining an operator chain, parses the string into nodes
// of the chain, and builds operators for each node. Currently nodes in a chain
// string are assumed to be separated by an underscore character ('_').
void Analysis::parseChainAndBuildOps(
   const std::string &OpChainStr
) {
   std::vector<std::string> ChainVec;
   std::stringstream OpChainSS(OpChainStr);

   std::string Part;

   while (std::getline(OpChainSS, Part, '_')) {
       ChainVec.push_back(Part);
   }

   std::string CurChainStr;
   for (const auto &ChainNode : ChainVec) {
      std::string Upstream = CurChainStr;
      if (CurChainStr.empty()) {
         CurChainStr = ChainNode;
      } else {
         CurChainStr += ("_" + ChainNode);
      }
//      std::cout << CurChainStr << " exists: " << Field::exists(CurChainStr) << " | ";
//      std::cout << Upstream << " " << CurChainStr << std::endl;
//      std::cout << "node,upstream,current: " << ChainNode << " " << Upstream << " " << CurChainStr << std::endl;

      if (!Field::exists(CurChainStr)) {
//         std::cout << "current does not exist" << std::endl;
         if (ChainNode.find("Spatial") != std::string::npos) {
//            std::cout << "creating " << ChainNode << " for " << Upstream << std::endl;
            registerAnalysisOp(ChainNode, {Upstream}, makeOpConfig());
         }
         if (ChainNode.find("Time") != std::string::npos) {
            std::size_t Pos = ChainNode.find_first_of("0123456789");
            if (Pos == std::string::npos) {
               ABORT_ERROR("Analysis: Improper temporal window string, {}", ChainNode);
            }
            std::string TimeOp = ChainNode.substr(0, Pos);
            std::string FreqStr = ChainNode.substr(Pos);
            registerAnalysisOp(TimeOp, {Upstream}, makeOpConfig(opParam("Period", FreqStr)));
         }
      }
   }

} // end parseChainAndBuildOps

//------------------------------------------------------------------------------
// For given operator type and upstream inputs, create an AnalysisOperator
// instance and an OperatorNode
void Analysis::registerAnalysisOp(
    const std::string &OpName,
    const std::vector<std::string> &UpstreamNames,
    Config Options
) {

   auto NewOp = AnalysisOpFactory::createOp(OpName, UpstreamNames, Options);
   std::string NewName = NewOp->getName();
//   std::cout << "register op w name:  " << NewName << "| exists: " << OpNodeExists(NewName) << std::endl;

   if (NewOp) {

      OperatorNode Node;
      Node.Op = std::move(NewOp);

      OpNodes.push_back(std::move(Node));
      RegisteredOpNames.push_back(NewName);

   }

//   std::cout << "registered: " << NewOp->getName() << std::endl;

} // end registerAnalysisOp

//------------------------------------------------------------------------------
void Analysis::buildOperatorDependencies() {
   for (auto &Node : OpNodes) {
      auto InputNames = Node.Op->getInputFieldNames();
      
      for (const auto &InputName : InputNames) {
         for (auto &PotentialUpstream : OpNodes) {
            auto UpstreamOutputs = PotentialUpstream.Op->getOutputFieldNames();
            
            for (const auto &UpstreamOutput : UpstreamOutputs) {
               if (UpstreamOutput == InputName) {
                  // Check if already in Upstreams vector
                  if (std::find(Node.Upstreams.begin(), 
                               Node.Upstreams.end(), 
                               &PotentialUpstream) == Node.Upstreams.end()) {
                     // Not found, so add it
                     Node.Upstreams.push_back(&PotentialUpstream);
                  }
                  break;
               }
            }
         }
      }
   }
}

//------------------------------------------------------------------------------
void Analysis::setComputeAlarms() {
   
   // Get the model timestep
   TimeInterval Timestep = ModelClock->getTimeStep();
   
   // Get current time
   TimeInstant CurrentTime = ModelClock->getCurrentTime();
   
   // Set alarms for terminal operators (those that produce output to be
   // written out with streams)
   for (auto &Node : OpNodes) {
      std::string OpName = Node.Op->getName();
      bool IsTimeMean = (OpName.find("TimeMean") != std::string::npos);
      
      if (!Node.StreamName.empty()) {
         // This is a terminal operator
         
         if (IsTimeMean) {
            // TimeMean operators compute every timestep for accumulation
            // Create a unique alarm owned by Analysis
            auto AccumulationAlarm = std::make_unique<Alarm>(
               "Compute_" + OpName, 
               Timestep,
               CurrentTime
            );

            // Attach alarm to clock
            ModelClock->attachAlarm(AccumulationAlarm.get());

            // Store pointer and transfer ownership
            Node.ComputeAlarms.push_back(AccumulationAlarm.get());
            AccumulationAlarms.push_back(std::move(AccumulationAlarm));
            
//            std::cout << "terminal avg op: " << OpName << std::endl;
         } else {
            // Discrete sampling operators: point to stream alarms
            for (const auto &StreamName : Node.StreamName) {
               auto StreamAlarm = IOStream::getAlarm(StreamName);
               
               // Check if already added (avoid duplicates)
               if (std::find(Node.ComputeAlarms.begin(), 
                            Node.ComputeAlarms.end(), 
                            StreamAlarm) == Node.ComputeAlarms.end()) {
                  Node.ComputeAlarms.push_back(StreamAlarm);

//                  std::cout << "terminal sampling op: " << OpName << std::endl;
               }
            }
         }
      }
   }
   
   // Propagate alarms upstream
   propagateAlarmsUpstream();
   
} // end setComputeAlarms

//------------------------------------------------------------------------------
void Analysis::propagateAlarmsUpstream() {
   
   // Iteratively propagate alarms from downstream to upstream operators
   // Continue until no more changes occur
   bool Changed = true;
   while (Changed) {
      Changed = false;
      
      for (auto &Node : OpNodes) {
         // Find all downstream operators (those that have Node as upstream)
         for (const auto &OtherNode : OpNodes) {
            for (const auto *Upstream : OtherNode.Upstreams) {
               if (Upstream == &Node) {
                  // Node is upstream of OtherNode
                  // Add OtherNode's alarms to Node (if not already present)
                  for (auto *DownstreamAlarm : OtherNode.ComputeAlarms) {
                     if (std::find(Node.ComputeAlarms.begin(), 
                                  Node.ComputeAlarms.end(), 
                                  DownstreamAlarm) == Node.ComputeAlarms.end()) {
                        Node.ComputeAlarms.push_back(DownstreamAlarm);
                        Changed = true;
//                        std::cout << "upstream node: " << Node.Op->getName() << " | " << OtherNode.Op->getName() << std::endl;
                     }
                  }
               }
            }
         }
      }
   }
   
} // end propagateAlarmsUpstream

//------------------------------------------------------------------------------
// Fetch a reference to the a pointer the ModelClock, required for
// IOStream::create.
Clock *&Analysis::getModelClock(){
   return ModelClock;
}

//------------------------------------------------------------------------------
const std::vector<OperatorNode*> Analysis::getOpNodes() {
   std::vector<OperatorNode*> OpPtrs;
   for (OperatorNode &OpNode: OpNodes) {
      OpPtrs.push_back(&OpNode);
   }
   return OpPtrs;
}

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
