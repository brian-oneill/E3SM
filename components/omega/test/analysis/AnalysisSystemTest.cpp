//===-- Test 5.2, 5.3, 5.4, 5.5, 5.6: Analysis system tests ----*- C++ -*-===//
//
// System tests for Analysis framework: dependency resolution, alarm system,
// factory registration, configuration parsing, and end-to-end integration
//
//===-----------------------------------------------------------------------===//

#include "Analysis.h"
#include "AnalysisOpFactory.h"
#include "AuxiliaryState.h"
#include "Decomp.h"
#include "Eos.h"
#include "Field.h"
#include "Halo.h"
#include "HorzMesh.h"
#include "IO.h"
#include "IOStream.h"
#include "Logging.h"
#include "OceanState.h"
#include "PGrad.h"
#include "Tendencies.h"
#include "TimeStepper.h"
#include "Tracers.h"
#include "VertAdv.h"
#include "VertCoord.h"

#include <iostream>

using namespace OMEGA;

// Test result tracking
int NumTests = 0;
int NumPassed = 0;
int NumFailed = 0;

//------------------------------------------------------------------------------
// Helper function to report test results
void reportTest(const std::string &TestName, bool Passed) {
   NumTests++;
   if (Passed) {
      NumPassed++;
      LOG_INFO("PASS: {}", TestName);
   } else {
      NumFailed++;
      LOG_ERROR("FAIL: {}", TestName);
   }
}

//===----------------------------------------------------------------------===//
// Test 5.2: Dependency Resolution and Execution Order
//===----------------------------------------------------------------------===//

//------------------------------------------------------------------------------
// Test 5.2.1: Verify shared intermediate operators are deduplicated
void testSharedIntermediates() {
   
   LOG_INFO("Testing shared intermediate deduplication...");
   
   auto DefAnalysis = Analysis::getDefault();
   auto OpNodes = DefAnalysis->getOpNodes();
   
   // Count how many times each operator name appears
   std::map<std::string, int> OpCounts;
   for (const auto *Node : OpNodes) {
      std::string OpName = Node->Op->getName();
      std::cout << " 5.2: " << OpName << " exists" << std::endl;
      OpCounts[OpName]++;
   }
   
   // Check that no operator appears more than once
   bool Passed = true;
   for (const auto &Pair : OpCounts) {
      if (Pair.second > 1) {
         LOG_ERROR("  Operator {} appears {} times (should be 1)", 
                   Pair.first, Pair.second);
         Passed = false;
      }
   }
   
   reportTest("Dependency: Shared intermediates deduplicated", Passed);
}

//------------------------------------------------------------------------------
// Test 5.2.2: Verify upstream dependencies are correctly resolved
void testUpstreamDependencies() {
   
   LOG_INFO("Testing upstream dependency resolution...");
   
   auto DefAnalysis = Analysis::getDefault();
   auto OpNodes = DefAnalysis->getOpNodes();
   
   bool Passed = true;
   
   // For each operator, verify its upstreams produce the fields it needs
   for (const auto *Node : OpNodes) {
      auto InputNames = Node->Op->getInputFieldNames();
      
      for (const auto &InputName : InputNames) {
         // Check if this input is a simulation field or an operator output
         bool FoundUpstream = false;
         
         // Check if it's a simulation field
         if (Field::exists(InputName)) {
            FoundUpstream = true;
         }
         
         // Check if it's produced by an upstream operator
         for (const auto *Upstream : Node->Upstreams) {
            auto UpstreamOutputs = Upstream->Op->getOutputFieldNames();
            for (const auto &Output : UpstreamOutputs) {
               if (Output == InputName) {
                  FoundUpstream = true;
                  break;
               }
            }
            if (FoundUpstream) break;
         }
         
         if (!FoundUpstream) {
            LOG_ERROR("  Operator {} requires input {} but no upstream found",
                      Node->Op->getName(), InputName);
            Passed = false;
         }
      }
   }
   
   reportTest("Dependency: Upstream dependencies resolved", Passed);
}

//------------------------------------------------------------------------------
// Test 5.2.3: Verify cache prevents redundant computation
void testCacheValidation() {
   
   LOG_INFO("Testing cache validation...");
   
   auto DefAnalysis = Analysis::getDefault();
   auto OpNodes = DefAnalysis->getOpNodes();
   
   if (OpNodes.empty()) {
      reportTest("Cache: Validation (no operators to test)", true);
      return;
   }

   auto AnalysisClock = DefAnalysis->getModelClock();
   auto CurTime = AnalysisClock->getCurrentTime();
   auto TimeStep = AnalysisClock->getTimeStep();

   auto TestTime = CurTime + TimeStep;
   
   // Get first operator
   auto *FirstNode = OpNodes[0];
//   TimeInstant TestTime(0, 0, 0, 0, 0, 1);
   
   // Initially should not be valid
   bool InitiallyInvalid = !FirstNode->Op->isCacheValid(TestTime);
   
   // Compute it
   FirstNode->Op->compute(TestTime);
   
   // Now should be valid for same timestamp
   bool NowValid = FirstNode->Op->isCacheValid(TestTime);
   
   // Should be invalid for different timestamp
   auto DifferentTime = TestTime + TimeStep;
//  TimeInstant DifferentTime(0, 0, 0, 0, 0, 2);
   bool InvalidForDifferentTime = !FirstNode->Op->isCacheValid(DifferentTime);
   
   bool Passed = InitiallyInvalid && NowValid && InvalidForDifferentTime;
   reportTest("Cache: Validation prevents redundant computation", Passed);
   
   if (!Passed) {
      LOG_ERROR("  InitiallyInvalid: {}, NowValid: {}, InvalidForDifferentTime: {}",
                InitiallyInvalid, NowValid, InvalidForDifferentTime);
   }
}

//===----------------------------------------------------------------------===//
// Test 5.3: Alarm System Verification
//===----------------------------------------------------------------------===//

//------------------------------------------------------------------------------
// Test 5.3.1: Verify terminal operators have alarms
void testTerminalOperatorAlarms() {
   
   LOG_INFO("Testing terminal operator alarms...");
   
   auto DefAnalysis = Analysis::getDefault();
   auto OpNodes = DefAnalysis->getOpNodes();
   
   bool Passed = true;
   
   // Terminal operators (those with StreamNames) should have alarms
   for (const auto *Node : OpNodes) {
      if (!Node->StreamNames.empty()) {
         if (Node->ComputeAlarms.empty()) {
            LOG_ERROR("  Terminal operator {} has no alarms",
                      Node->Op->getName());
            Passed = false;
         }
      }
   }
   
   reportTest("Alarm: Terminal operators have alarms", Passed);
}

//------------------------------------------------------------------------------
// Test 5.3.2: Verify temporal operators have multiple alarms
void testTemporalOperatorAlarms() {
   
   LOG_INFO("Testing temporal operator alarms...");
   
   auto DefAnalysis = Analysis::getDefault();
   auto OpNodes = DefAnalysis->getOpNodes();
   
   bool Passed = true;
   
   // Temporal reduction operators should have 2 alarms (accumulation + output)
   for (const auto *Node : OpNodes) {
      std::string OpType = Node->Op->getOperatorType();
      bool IsTimeReduction = (OpType.find("Time") != std::string::npos);
      
      if (IsTimeReduction && !Node->StreamNames.empty()) {
         if (Node->ComputeAlarms.size() < 2) {
            LOG_ERROR("  Temporal operator {} has {} alarms (expected 2)",
                      Node->Op->getName(), Node->ComputeAlarms.size());
            Passed = false;
         }
      }
   }
   
   reportTest("Alarm: Temporal operators have accumulation + output alarms", Passed);
}

//------------------------------------------------------------------------------
// Test 5.3.3: Verify alarm propagation to upstream operators
void testAlarmPropagation() {
   
   LOG_INFO("Testing alarm propagation...");
   
   auto DefAnalysis = Analysis::getDefault();
   auto OpNodes = DefAnalysis->getOpNodes();
   
   bool Passed = true;
   
   // Intermediate operators should have alarms propagated from downstream
   for (const auto *Node : OpNodes) {
      if (Node->StreamNames.empty() && !Node->Upstreams.empty()) {
         // This is an intermediate operator
         if (Node->ComputeAlarms.empty()) {
            LOG_ERROR("  Intermediate operator {} has no propagated alarms",
                      Node->Op->getName());
            Passed = false;
         }
      }
   }
   
   reportTest("Alarm: Propagation to upstream operators", Passed);
}

//===----------------------------------------------------------------------===//
// Test 5.4: Factory Registration and Type Dispatch
//===----------------------------------------------------------------------===//

//------------------------------------------------------------------------------
// Helper: Create a test field for factory testing
void createFactoryTestField(const std::string &FieldName,
                            const HorzMesh *Mesh,
                            const VertCoord *VCoord) {
   
   I4 NCells = Mesh->NCellsSize;
   I4 NVertLevels = VCoord->NVertLevels;
   
   // Create a 2D test field
   std::vector<std::string> DimNames = {"NCells", "NVertLevels"};
   auto TestField = Field::create(
      FieldName,
      "Test field for factory validation",
      "units",
      "",
      -1.0e30,
      1.0e30,
      -1.0e30,
      2,
      DimNames
   );
   
   // Allocate and attach data with known values
   Array2DReal TestData(FieldName + "_data", NCells, NVertLevels);
   TestField->attachData<Array2DReal>(TestData);
   
   // Fill with simple pattern: value = Cell + K
   auto TestDataHost = Kokkos::create_mirror_view(TestData);
   for (I4 Cell = 0; Cell < NCells; ++Cell) {
      for (I4 K = 0; K < NVertLevels; ++K) {
         TestDataHost(Cell, K) = static_cast<Real>(Cell + K);
      }
   }
   Kokkos::deep_copy(TestData, TestDataHost);
}

//------------------------------------------------------------------------------
// Test 5.4.1: Verify all base operators are registered
void testFactoryRegistration() {
   
   LOG_INFO("Testing factory registration...");
   
   // Check that all required operators are registered
   std::vector<std::string> RequiredOps = {
      "SpatialMax",
      "SpatialMin",
      "SpatialMean",
      "SpatialStdDev",
      "TimeMean"
   };
   
   bool AllRegistered = true;
   for (const auto &OpName : RequiredOps) {
      if (!AnalysisOpFactory::hasOperator(OpName)) {
         LOG_ERROR("  Operator {} not registered", OpName);
         AllRegistered = false;
      }
   }
   
   reportTest("Factory: All base operators registered", AllRegistered);
}

//------------------------------------------------------------------------------
// Test 5.4.2: Verify type dispatch and operator creation
void testFactoryTypeDispatch(const MachEnv *Env,
                             const HorzMesh *Mesh,
                             const VertCoord *VCoord) {
   
   LOG_INFO("Testing factory type dispatch...");
   
   // Create a test field specifically for this test
   std::string TestFieldName = "FactoryTestField";
   createFactoryTestField(TestFieldName, Mesh, VCoord);
   
   Config EmptyConfig;
   bool Passed = true;
   
   try {
      // Create operator using the factory
      auto MaxOp = AnalysisOpFactory::createOp("SpatialMax",
                                               {TestFieldName},
                                               EmptyConfig);
      if (!MaxOp) {
         LOG_ERROR("  Failed to create SpatialMax operator");
         Passed = false;
      } else {
         // Verify operator type is correct
         if (MaxOp->getOperatorType() != "SpatialMax") {
            LOG_ERROR("  Operator type mismatch: expected SpatialMax, got {}",
                      MaxOp->getOperatorType());
            Passed = false;
         }
         
         // Initialize and compute to verify full functionality
         MaxOp->initialize(Env, Mesh, VCoord, EmptyConfig);
         
         TimeInstant TestTime(0, 0, 0, 0, 0, 0);
         MaxOp->compute(TestTime);
         
         // Verify output field was created
         std::string OutputFieldName = TestFieldName + "_SpatialMax";
         if (!Field::exists(OutputFieldName)) {
            LOG_ERROR("  Output field {} was not created", OutputFieldName);
            Passed = false;
         } else {
            // Verify output contains expected value
            auto ResultField = Field::get(OutputFieldName);
            auto ResultData = ResultField->getDataArray<Array1DReal>();
            auto ResultHost = Kokkos::create_mirror_view(ResultData);
            Kokkos::deep_copy(ResultHost, ResultData);
            
            // Expected max: (NCells-1) + (NVertLevels-1)
            Real ExpectedMax = static_cast<Real>(Mesh->NCellsSize - 1 + 
                                                 VCoord->NVertLevels - 1);
            Real ComputedMax = ResultHost(0);
            
            if (std::abs(ComputedMax - ExpectedMax) > 1.0e-10) {
               LOG_ERROR("  Computed max {} differs from expected {}",
                        ComputedMax, ExpectedMax);
               Passed = false;
            }
         }
      }
   } catch (const std::exception &e) {
      LOG_ERROR("  Exception during operator creation/execution: {}", e.what());
      Passed = false;
   }
   
   reportTest("Factory: Type dispatch creates functional operator", Passed);
}

//------------------------------------------------------------------------------
// Test 5.4.3: Verify error handling for invalid operator types
void testFactoryErrorHandling(const HorzMesh *Mesh,
                              const VertCoord *VCoord) {
   
   LOG_INFO("Testing factory error handling...");
   
   // Create a test field for error testing
   std::string TestFieldName = "FactoryErrorTestField";
   createFactoryTestField(TestFieldName, Mesh, VCoord);
   
   Config EmptyConfig;
   bool Passed = true;
   
   // Test 1: Try to create an operator that doesn't exist
   try {
      auto InvalidOp = AnalysisOpFactory::createOp("NonExistentOperator",
                                                    {TestFieldName},
                                                    EmptyConfig);
      if (InvalidOp) {
         LOG_ERROR("  Factory created operator for invalid type");
         Passed = false;
      }
      // If we get here without exception and InvalidOp is null, that's acceptable
   } catch (...) {
      // Expected behavior - exception thrown for invalid operator
      // This is also acceptable error handling
   }
   
   // Test 2: Try to create operator with non-existent field
   try {
      auto Op2 = AnalysisOpFactory::createOp("SpatialMax",
                                             {"NonExistentField"},
                                             EmptyConfig);
      if (Op2) {
         LOG_ERROR("  Factory created operator for non-existent field");
         Passed = false;
      }
   } catch (...) {
      // Expected behavior - should fail gracefully
   }
   
   reportTest("Factory: Error handling for invalid inputs", Passed);
}

//------------------------------------------------------------------------------
// Test 5.4.4: Verify factory instantiates all operator types
void testFactoryInstantiateAll(const MachEnv *Env,
                               const HorzMesh *Mesh,
                               const VertCoord *VCoord) {
   
   LOG_INFO("Testing factory instantiation for all operator types...");
   
   // Create a test field for all operators
   std::string TestFieldName = "FactoryInstantiateTestField";
   createFactoryTestField(TestFieldName, Mesh, VCoord);
   
   std::vector<std::string> SpatialOps = {
      "SpatialMax",
      "SpatialMin",
      "SpatialMean",
      "SpatialStdDev"
   };
   
   Config EmptyConfig;
   bool Passed = true;
   
   // Test each spatial operator
   for (const auto &OpName : SpatialOps) {
      try {
         auto Op = AnalysisOpFactory::createOp(OpName,
                                               {TestFieldName},
                                               EmptyConfig);
         if (!Op) {
            LOG_ERROR("  Failed to create {} operator", OpName);
            Passed = false;
            continue;
         }
         
         // Initialize and compute to verify full functionality
         Op->initialize(Env, Mesh, VCoord, EmptyConfig);
         
         TimeInstant TestTime(0, 0, 0, 0, 0, 0);
         Op->compute(TestTime);
         
         // Verify output field was created
         auto OutputNames = Op->getOutputFieldNames();
         if (OutputNames.empty()) {
            LOG_ERROR("  Operator {} produced no output fields", OpName);
            Passed = false;
         } else {
            for (const auto &OutputName : OutputNames) {
               if (!Field::exists(OutputName)) {
                  LOG_ERROR("  Output field {} does not exist", OutputName);
                  Passed = false;
               }
            }
         }
      } catch (const std::exception &e) {
         LOG_ERROR("  Exception creating/executing {} operator: {}", 
                   OpName, e.what());
         Passed = false;
      }
   }
   
   // Test TimeMean with required Period parameter
   try {
      Config TimeMeanConfig;
      TimeMeanConfig.set("Period", std::string("1day"));
      auto TimeMeanOp = AnalysisOpFactory::createOp("TimeMean",
                                                     {TestFieldName},
                                                     TimeMeanConfig);
      if (!TimeMeanOp) {
         LOG_ERROR("  Failed to create TimeMean operator");
         Passed = false;
      } else {
         // Initialize to verify it doesn't crash
         TimeMeanOp->initialize(Env, Mesh, VCoord, TimeMeanConfig);
         
         // Verify output field naming
         auto OutputNames = TimeMeanOp->getOutputFieldNames();
         if (OutputNames.empty()) {
            LOG_ERROR("  TimeMean operator produced no output fields");
            Passed = false;
         }
      }
   } catch (const std::exception &e) {
      LOG_ERROR("  Exception creating/executing TimeMean operator: {}", e.what());
      Passed = false;
   }
   
   reportTest("Factory: Instantiate and execute all operator types", Passed);
}

//===----------------------------------------------------------------------===//
// Test 5.5: Configuration Parsing and Validation
//===----------------------------------------------------------------------===//

//------------------------------------------------------------------------------
// Test 5.5.1: Verify operator chain parsing
void testOperatorChainParsing() {
   
   LOG_INFO("Testing operator chain parsing...");
   
   // This test verifies that parseChainAndBuildOps correctly handles
   // underscore-delimited operator chains
   
   auto DefAnalysis = Analysis::getDefault();
   
   // Check if any operators were created from config
   auto OpNodes = DefAnalysis->getOpNodes();
   bool Passed = !OpNodes.empty();
   
   reportTest("Config: Operator chain parsing", Passed);
   
   if (!Passed) {
      LOG_ERROR("  No operators were created from configuration");
   }
}

//------------------------------------------------------------------------------
// Test 5.5.2: Verify field reuse in chains
void testFieldReuseInChains() {
   
   LOG_INFO("Testing field reuse in operator chains...");
   
   auto DefAnalysis = Analysis::getDefault();
   auto OpNodes = DefAnalysis->getOpNodes();
   
   // Count unique output field names
   std::set<std::string> UniqueOutputs;
   for (const auto *Node : OpNodes) {
      auto Outputs = Node->Op->getOutputFieldNames();
      for (const auto &Output : Outputs) {
         UniqueOutputs.insert(Output);
      }
   }
   
   // Each operator should produce a unique output
   bool Passed = (UniqueOutputs.size() == OpNodes.size());
   reportTest("Config: Field reuse prevents duplicates", Passed);
   
   if (!Passed) {
      LOG_ERROR("  {} unique outputs for {} operators",
                UniqueOutputs.size(), OpNodes.size());
   }
}

//------------------------------------------------------------------------------
// Test 5.5.3: Verify stream parameter application
void testStreamParameterApplication() {
   
   LOG_INFO("Testing stream parameter application...");
   
   // Verify that streams were created for analysis output
   // This is a basic check that the stream creation succeeded
   
   auto DefAnalysis = Analysis::getDefault();
   auto OpNodes = DefAnalysis->getOpNodes();
   
   bool Passed = false;
   
   // Check if any terminal operators have associated streams
   for (const auto *Node : OpNodes) {
      if (!Node->StreamNames.empty()) {
         Passed = true;
         break;
      }
   }
   
   reportTest("Config: Stream parameters applied", Passed);
   
   if (!Passed) {
      LOG_ERROR("  No terminal operators with associated streams found");
   }
}

//===----------------------------------------------------------------------===//
// Test 5.6: End-to-End Integration
//===----------------------------------------------------------------------===//

//------------------------------------------------------------------------------
// Test 5.6.1: Verify computeAll executes without errors
void testComputeAllExecution(Clock *ModelClock) {
   
   LOG_INFO("Testing computeAll execution...");
   
   auto DefAnalysis = Analysis::getDefault();
   
   bool Passed = true;

   // Advance clock to trigger alarms
   TimeInstant CurrentTime = ModelClock->getCurrentTime();
   TimeInterval OneStep = ModelClock->getTimeStep();
   TimeInstant NextTime = CurrentTime + OneStep;
   ModelClock->advance();
   
   // Call computeAll
   DefAnalysis->computeAll();
      
   reportTest("Integration: computeAll executes without errors", Passed);
}

//------------------------------------------------------------------------------
// Test 5.6.2: Verify output fields are created
void testOutputFieldsCreated() {
   
   LOG_INFO("Testing output field creation...");
   
   auto DefAnalysis = Analysis::getDefault();
   auto OpNodes = DefAnalysis->getOpNodes();
   
   bool Passed = true;
   
   // Verify that all operator output fields exist in Field registry
   for (const auto *Node : OpNodes) {
      auto Outputs = Node->Op->getOutputFieldNames();
      for (const auto &Output : Outputs) {
         if (!Field::exists(Output)) {
            LOG_ERROR("  Output field {} does not exist", Output);
            Passed = false;
         }
      }
   }
   
   reportTest("Integration: Output fields created", Passed);
}

//------------------------------------------------------------------------------
// Test 5.6.3: Verify stream output (basic check)
void testStreamOutput() {
   
   LOG_INFO("Testing stream output...");
   
   // This is a basic check that streams are configured
   // Full I/O testing would require writing and reading files
   
   auto DefAnalysis = Analysis::getDefault();
   auto OpNodes = DefAnalysis->getOpNodes();
   
   bool Passed = false;
   
   // Check if any operators are associated with streams
   for (const auto *Node : OpNodes) {
      if (!Node->StreamNames.empty()) {
         // Verify the stream exists
         for (const auto &StreamName : Node->StreamNames) {
            auto NodeStream = IOStream::get(StreamName);
            if (NodeStream) {
               Passed = true;
               break;
            }
         }
      }
      if (Passed) break;
   }
   
   reportTest("Integration: Stream output configured", Passed);
   
   if (!Passed) {
      LOG_ERROR("  No valid streams found for analysis output");
   }
}

//===----------------------------------------------------------------------===//
// Main test driver
//===----------------------------------------------------------------------===//

int main(int argc, char *argv[]) {
   
   int ErrCode = 0;
   
   MPI_Init(&argc, &argv);
   Kokkos::initialize();
   Pacer::initialize(MPI_COMM_WORLD);
   Pacer::setPrefix("Omega:");
   {
      // Initialize full Omega infrastructure for integration tests
      MachEnv::init(MPI_COMM_WORLD);
      MachEnv *DefEnv = MachEnv::getDefault();
      MPI_Comm DefComm = DefEnv->getComm();
      
      initLogging(DefEnv);
      
      LOG_INFO("=======================================================");
      LOG_INFO("Analysis System Tests (5.2, 5.3, 5.4, 5.5, 5.6)");
      LOG_INFO("=======================================================");
      
      Config("Omega");
      Config::readAll("omega.yml");
      
      TimeStepper::init1();
      TimeStepper *DefStepper = TimeStepper::getDefault();
      Clock *ModelClock = DefStepper->getClock();
      
      IO::init(DefComm);
      Decomp::init();
      IOStream::init(ModelClock);
      Field::init(ModelClock);
      Halo::init();
      HorzMesh::init();
      VertCoord::init();
      Tracers::init();
      AuxiliaryState::init();
      Eos::init();
      PressureGrad::init();
      Tendencies::init();
      VertAdv::init();
      TimeStepper::init2();
      OceanState::init();
      
      // Validate streams
      bool StreamsValid = IOStream::validateAll();
      if (!StreamsValid) {
         LOG_ERROR("Stream validation failed");
      }
      
      // Read initial state
      Metadata ReqMeta;
      Error Err1 = IOStream::read("InitialState", ModelClock, ReqMeta);
      if (Err1.isFail()) {
         LOG_ERROR("Failed to read initial state");
      }
      
      // Initialize Analysis module (creates operators, resolves dependencies, sets alarms)
      Analysis::init();
      
      LOG_INFO("");
      LOG_INFO("--- Test 5.2: Dependency Resolution and Execution Order ---");
      testSharedIntermediates();
      testUpstreamDependencies();
      testCacheValidation();
      
      LOG_INFO("");
      LOG_INFO("--- Test 5.3: Alarm System Verification ---");
      testTerminalOperatorAlarms();
      testTemporalOperatorAlarms();
      testAlarmPropagation();
      
      LOG_INFO("");
      LOG_INFO("--- Test 5.4: Factory Registration and Type Dispatch ---");
      auto Mesh = HorzMesh::getDefault();
      auto VCoord = VertCoord::getDefault();
      testFactoryRegistration();
      testFactoryTypeDispatch(DefEnv, Mesh, VCoord);
      testFactoryErrorHandling(Mesh, VCoord);
      testFactoryInstantiateAll(DefEnv, Mesh, VCoord);
      
      LOG_INFO("");
      LOG_INFO("--- Test 5.5: Configuration Parsing and Validation ---");
      testOperatorChainParsing();
      testFieldReuseInChains();
      testStreamParameterApplication();
      
      LOG_INFO("");
      LOG_INFO("--- Test 5.6: End-to-End Integration ---");
      testComputeAllExecution(ModelClock);
      testOutputFieldsCreated();
      testStreamOutput();
      
      // Report summary
      LOG_INFO("");
      LOG_INFO("=======================================================");
      LOG_INFO("Test Summary:");
      LOG_INFO("  Total tests: {}", NumTests);
      LOG_INFO("  Passed: {}", NumPassed);
      LOG_INFO("  Failed: {}", NumFailed);
      LOG_INFO("=======================================================");
      
      if (NumFailed > 0) {
         ErrCode = 1;
      }
      
      // Cleanup
      Analysis::finalize();
      IOStream::finalize();
      OceanState::clear();
      Tracers::clear();
      AuxiliaryState::clear();
      PressureGrad::clear();
      Tendencies::clear();
      VertAdv::clear();
      VertCoord::clear();
      TimeStepper::clear();
      HorzMesh::clear();
      Field::clear();
      Dimension::clear();
      Halo::clear();
      Decomp::clear();
      MachEnv::removeAll();
   }
   Pacer::finalize();
   Kokkos::finalize();
   MPI_Finalize();
   
   return ErrCode;
}
