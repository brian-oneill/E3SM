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

   auto DefEnv  = MachEnv::getDefault();

   auto Mesh = HorzMesh::getDefault();

   auto VCoord = VertCoord::getDefault();

   auto DefTimeStepper = TimeStepper::getDefault();

   Clock *OmegaClock = DefTimeStepper->getClock();

   Config *OmegaConfig = Config::getOmegaConfig();

   Analysis::DefAnalysis =
       create("Default", DefEnv, Mesh, VCoord, OmegaClock, OmegaConfig);

//   IOStream::printAllStreams();
} // end init

//------------------------------------------------------------------------------
// Creates a new Analysis instance using the constructor and puts it in the
// AllAnalysisObjects map.
Analysis *Analysis::create(
   const std::string &Name,
   const MachEnv *Env,
   const HorzMesh *Mesh,
   const VertCoord *VCoord,
   Clock *ModelClock,
   Config *Options
) {

   if (AllAnalysisObjects.find(Name) != AllAnalysisObjects.end()) {
      LOG_ERROR("Attempted to create an Analysis with name {} "
                "but one with that name already exists", Name);
      return nullptr;
   }

   auto NewAnalysis = new Analysis(Name, Env, Mesh, VCoord, ModelClock, Options);

   AllAnalysisObjects.emplace(Name, NewAnalysis);

   return NewAnalysis;

} // end create


//------------------------------------------------------------------------------
// Construct a new Analysis instance. Loops over the contents of the Analysis
// node in the config file, calls derived constructor for pre-defined
// AnalysisGroups, or parses composable operator chains for custom groups.
Analysis::Analysis(const std::string &InName,
                        const MachEnv *InEnv,
                        const HorzMesh *InMesh,
                        const VertCoord *InVCoord,
                        Clock *InModelClock,
                        Config *Options) {

   Name = InName;
   Env = InEnv;
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

//   for (Config::Iter It = AnalysisCfg.begin(); It != AnalysisCfg.end(); ++It) {
//      std::string GroupName;
//      AnalysisCfg.getName(It, GroupName);
//      Config GroupCfg(GroupName);
//      Err += AnalysisCfg.get(GroupCfg);
//      bool GroupEnabled = false;
//      GroupCfg.get("Enable", GroupEnabled);
//      if (GroupEnabled) {
////         std::cout << GroupName << " " << GroupEnabled << std::endl;
//         if (GroupName == "GlobalStats") {
//            GlobalStats GlobalStatsGroup(NamePrefix + GroupName, GroupCfg, this);
//
//            continue;
//         }
//         ABORT_ERROR("Analysis: custom analysis group enabled in config, but"
//                     "composable operators are not yet supported.");
//      }
//   } 
//
//   buildOperatorDependencies();
//
//   setComputeAlarms();
//
//   initializeAllOps();

} // end constructor

//------------------------------------------------------------------------------
// For a given string defining an operator chain, parses the string into nodes
// of the chain, and builds operators for each node. Currently nodes in a chain
// string are assumed to be separated by an underscore character ('_').
void Analysis::parseChainAndBuildOps(
   const std::string &OpChainStr
) {

   // The input OpChainStr is broken into nodes and stored in ChainVec
   std::vector<std::string> ChainVec;
   std::stringstream OpChainSS(OpChainStr);
   std::string Part;
   while (std::getline(OpChainSS, Part, '_')) {
       ChainVec.push_back(Part);
   }

   // Working from the beginning of the chain, check if the Op already exists
   // by checking the Field registry for the Op output. If not build the Op and
   // Node by calling registerAnalysisOp
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
// instance and places it within a new OperatorNode. The OperatorNode is placed
// within the OpNodes member of the Analysis class.
void Analysis::registerAnalysisOp(
    const std::string &OpName,
    const std::vector<std::string> &UpstreamNames,
    Config Options
) {

   auto NewOp = AnalysisOpFactory::createOp(OpName, UpstreamNames, Options);
   std::string NewName = NewOp->getName();
//   std::cout << "register op w name:  " << NewName << "| exists: " << OpNodeExists(NewName) << std::endl;

   if (NewOp) {
      auto Node = std::make_unique<OperatorNode>();
      Node->Op = std::move(NewOp);
      
      OpNodes.push_back(std::move(Node));



//      OperatorNode Node;
//      Node.Op = std::move(NewOp);

//      OpNodes.push_back(std::move(Node));

   }

//   std::cout << "registered: " << NewOp->getName() << std::endl;

} // end registerAnalysisOp

//------------------------------------------------------------------------------
void Analysis::buildOperatorDependencies() {
   for (auto &Node : OpNodes) {
      auto InputNames = Node->Op->getInputFieldNames();
      
      for (const auto &InputName : InputNames) {
         for (auto &PotentialUpstream : OpNodes) {
            auto UpstreamOutputs = PotentialUpstream->Op->getOutputFieldNames();
            
            for (const auto &UpstreamOutput : UpstreamOutputs) {
               if (UpstreamOutput == InputName) {
                  // Check if already in Upstreams vector
                  if (std::find(Node->Upstreams.begin(), 
                               Node->Upstreams.end(), 
                               PotentialUpstream.get()) == Node->Upstreams.end()) {
                     // Not found, so add it
                     Node->Upstreams.push_back(PotentialUpstream.get());
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
      std::string OpType = Node->Op->getOperatorType();
      bool IsTimeReduction = (OpType.find("Time") != std::string::npos);
      
      if (!Node->StreamNames.empty()) {
         // This is a terminal operator
         
         if (IsTimeReduction) {
            // TimeMean operators compute every timestep for accumulation
            // Create a unique alarm owned by Analysis
            auto AccumulationAlarm = std::make_unique<Alarm>(
               "Compute_" + OpType, 
               Timestep,
               CurrentTime
            );

            // Attach alarm to clock
            ModelClock->attachAlarm(AccumulationAlarm.get());

            // Store pointer and transfer ownership
            Node->ComputeAlarms.push_back(AccumulationAlarm.get());
            AccumulationAlarms.push_back(std::move(AccumulationAlarm));
            
            // Time reduction operators have exactly one associated stream
            // Add the stream's output alarm for finalization
            auto StreamAlarm = IOStream::getAlarm(Node->StreamNames[0]);
            Node->ComputeAlarms.push_back(StreamAlarm);
            
            // Give the operator a pointer to the period alarm for finalization
            Node->Op->setPeriodAlarm(StreamAlarm);
            
//            std::cout << "terminal time reduction op: " << OpType << std::endl;
         } else {
            // Discrete sampling operators: point to stream alarms
            for (const auto &StreamName : Node->StreamNames) {
               auto StreamAlarm = IOStream::getAlarm(StreamName);
               
               // Check if already added (avoid duplicates)
               if (std::find(Node->ComputeAlarms.begin(), 
                            Node->ComputeAlarms.end(), 
                            StreamAlarm) == Node->ComputeAlarms.end()) {
                  Node->ComputeAlarms.push_back(StreamAlarm);

//                  std::cout << "terminal sampling op: " << OpType << std::endl;
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
            for (const auto *Upstream : OtherNode->Upstreams) {
               if (Upstream == Node.get()) {
                  // Node is upstream of OtherNode
                  // Add OtherNode's alarms to Node (if not already present)
                  for (auto *DownstreamAlarm : OtherNode->ComputeAlarms) {
                     if (std::find(Node->ComputeAlarms.begin(), 
                                  Node->ComputeAlarms.end(), 
                                  DownstreamAlarm) == Node->ComputeAlarms.end()) {
                        Node->ComputeAlarms.push_back(DownstreamAlarm);
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
// Main computational loop called every timestep. Checks all operator nodes
// to see if any of their alarms are ringing, and if so, calls computeRecursive
// to ensure the operator and all its upstream dependencies are computed.
void Analysis::computeAll() {
   
   TimeInstant CurrentTime = ModelClock->getCurrentTime();
   
   for (auto &Node : OpNodes) {
      // Check if any alarm is ringing for this operator
      bool ShouldCompute = false;
      for (auto *Alarm : Node->ComputeAlarms) {
         if (Alarm->isRinging()) {
            ShouldCompute = true;
            break;
         }
      }
      
      if (ShouldCompute) {
         computeRecursive(Node.get(), CurrentTime);
      }
   }
   
} // end computeAll

//------------------------------------------------------------------------------
// Recursively compute an operator node and all its upstream dependencies.
// Uses cache validation to avoid redundant computation within a timestep.
void Analysis::computeRecursive(OperatorNode *Node, const TimeInstant &TimeStamp) {
   
   // Check if already computed for this timestep (cache hit)
   if (Node->Op->isCacheValid(TimeStamp)) {
      return;
   }
   
   // Recursively compute all upstream dependencies first
   for (auto *Upstream : Node->Upstreams) {
      computeRecursive(Upstream, TimeStamp);
   }
   
   // Now compute this operator
   Node->Op->compute(TimeStamp);
   
} // end computeRecursive

//------------------------------------------------------------------------------
//
void Analysis::initializeAllOps() {

   for (auto &OpNode: OpNodes) {
      OpNode->Op->initialize( Env, Mesh, VCoord, makeOpConfig());
   }

} // end initializeAllOps

//------------------------------------------------------------------------------
// Fetch a reference to the a pointer the ModelClock, required for
// IOStream::create.
Clock *&Analysis::getModelClock(){
   return ModelClock;
}

//------------------------------------------------------------------------------
const std::vector<OperatorNode*> Analysis::getOpNodes() {
   std::vector<OperatorNode*> OpPtrs;
   for (auto &Node: OpNodes) {
      OpPtrs.push_back(Node.get());
   }
   return OpPtrs;
}

//------------------------------------------------------------------------------
bool Analysis::OpNodeExists(const std::string &FullOpName) {

   for (const auto &Node: OpNodes) {
      if (FullOpName == Node->Op->getName()) {
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
