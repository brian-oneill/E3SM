//===-- Test 5.1 & 5.4: Operator correctness and factory tests -*- C++ -*-===//
//
// Unit tests for Analysis operators with known-answer solutions
// and factory registration/type dispatch validation
//
//===-----------------------------------------------------------------------===//

#include "Analysis.h"
#include "AnalysisOpFactory.h"
#include "Decomp.h"
#include "Field.h"
#include "Halo.h"
#include "HorzMesh.h"
#include "IO.h"
#include "Logging.h"
#include "TimeStepper.h"
#include "VertCoord.h"

#include <cmath>
#include <iostream>

using namespace OMEGA;

// Test result tracking
int NumTests = 0;
int NumPassed = 0;
int NumFailed = 0;

// Tolerance for floating point comparisons
constexpr Real Tolerance = 1.0e-10;

//------------------------------------------------------------------------------
// Helper function to check if two values are approximately equal
bool approxEqual(Real a, Real b, Real tol = Tolerance) {
   return std::abs(a - b) < tol;
}

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

//------------------------------------------------------------------------------
// Create a simple test field with known values
void createTestField(const std::string &FieldName, 
                     const HorzMesh *Mesh,
                     const VertCoord *VCoord,
                     Real (*ValueFunc)(I4, I4)) {
   
   I4 NCells = Mesh->NCellsAll;
   I4 NVertLevels = VCoord->NVertLevels;
   
   // Create field
   std::vector<std::string> DimNames = {"NCells", "NVertLevels"};
   auto TestField = Field::create(
      FieldName,
      "Test field for operator validation",
      "m",
      "",
      -1.0e30,
      1.0e30,
      -1.0e30,
      2,
      DimNames
   );
   
   // Allocate and attach data
   Array2DReal TestData(FieldName + "_data", NCells, NVertLevels);
   TestField->attachData<Array2DReal>(TestData);
   
   // Fill with known values
   auto TestDataHost = Kokkos::create_mirror_view(TestData);
   for (I4 Cell = 0; Cell < NCells; ++Cell) {
      for (I4 K = 0; K < NVertLevels; ++K) {
         TestDataHost(Cell, K) = ValueFunc(Cell, K);
      }
   }
   Kokkos::deep_copy(TestData, TestDataHost);
}

//===----------------------------------------------------------------------===//
// Test 5.1: Individual Operator Correctness
//===----------------------------------------------------------------------===//

//------------------------------------------------------------------------------
// Test 5.1.1: SpatialMaxOp with known values
void testSpatialMaxOp(const MachEnv *Env, 
                      const HorzMesh *Mesh,
                      const VertCoord *VCoord) {
   
   LOG_INFO("Testing SpatialMaxOp...");
   
   // Create test field: values = Cell + K
   // Max value will be (NCells-1) + (NVertLevels-1)
   auto valueFunc = [](I4 Cell, I4 K) -> Real {
      return static_cast<Real>(Cell + K);
   };
   
   createTestField("TestFieldMax", Mesh, VCoord, valueFunc);
   
   // Expected max value
   Real ExpectedMax = static_cast<Real>(Mesh->NCellsAll - 1 + 
                                        VCoord->NVertLevels - 1);
   
   // Create and compute operator
   Config EmptyConfig;
   auto MaxOp = AnalysisOpFactory::createOp("SpatialMax", 
                                            {"TestFieldMax"}, 
                                            EmptyConfig);
   MaxOp->initialize(Env, Mesh, VCoord, EmptyConfig);
   
   TimeInstant TestTime;
   MaxOp->compute(TestTime);
   
   // Get result
   auto ResultField = Field::get("TestFieldMax_SpatialMax");
   auto ResultData = ResultField->getDataArray<Array1DReal>();
   auto ResultHost = Kokkos::create_mirror_view(ResultData);
   Kokkos::deep_copy(ResultHost, ResultData);
   
   Real ComputedMax = ResultHost(0);
   
   // Verify
   bool Passed = approxEqual(ComputedMax, ExpectedMax);
   reportTest("SpatialMaxOp: Known maximum value", Passed);
   
   if (!Passed) {
      LOG_ERROR("  Expected: {}, Got: {}", ExpectedMax, ComputedMax);
   }
}

//------------------------------------------------------------------------------
// Test 5.1.2: SpatialMinOp with known values
void testSpatialMinOp(const MachEnv *Env,
                      const HorzMesh *Mesh,
                      const VertCoord *VCoord) {
   
   LOG_INFO("Testing SpatialMinOp...");
   
   // Create test field: values = Cell + K + 100
   // Min value will be 0 + 0 + 100 = 100
   auto valueFunc = [](I4 Cell, I4 K) -> Real {
      return static_cast<Real>(Cell + K + 100);
   };
   
   createTestField("TestFieldMin", Mesh, VCoord, valueFunc);
   
   Real ExpectedMin = 100.0;
   
   // Create and compute operator
   Config EmptyConfig;
   auto MinOp = AnalysisOpFactory::createOp("SpatialMin",
                                            {"TestFieldMin"},
                                            EmptyConfig);
   MinOp->initialize(Env, Mesh, VCoord, EmptyConfig);
   
   TimeInstant TestTime;
   MinOp->compute(TestTime);
   
   // Get result
   auto ResultField = Field::get("TestFieldMin_SpatialMin");
   auto ResultData = ResultField->getDataArray<Array1DReal>();
   auto ResultHost = Kokkos::create_mirror_view(ResultData);
   Kokkos::deep_copy(ResultHost, ResultData);
   
   Real ComputedMin = ResultHost(0);
   
   // Verify
   bool Passed = approxEqual(ComputedMin, ExpectedMin);
   reportTest("SpatialMinOp: Known minimum value", Passed);
   
   if (!Passed) {
      LOG_ERROR("  Expected: {}, Got: {}", ExpectedMin, ComputedMin);
   }
}

//------------------------------------------------------------------------------
// Test 5.1.3: SpatialMeanOp with known values
void testSpatialMeanOp(const MachEnv *Env,
                       const HorzMesh *Mesh,
                       const VertCoord *VCoord) {
   
   LOG_INFO("Testing SpatialMeanOp...");
   
   // Create test field with constant value = 42.0
   // Mean should be exactly 42.0
   auto valueFunc = [](I4 Cell, I4 K) -> Real {
      return 42.0;
   };
   
   createTestField("TestFieldMean", Mesh, VCoord, valueFunc);
   
   Real ExpectedMean = 42.0;
   
   // Create and compute operator
   Config EmptyConfig;
   auto MeanOp = AnalysisOpFactory::createOp("SpatialMean",
                                             {"TestFieldMean"},
                                             EmptyConfig);
   MeanOp->initialize(Env, Mesh, VCoord, EmptyConfig);
   
   TimeInstant TestTime;
   MeanOp->compute(TestTime);
   
   // Get result
   auto ResultField = Field::get("TestFieldMean_SpatialMean");
   auto ResultData = ResultField->getDataArray<Array1DReal>();
   auto ResultHost = Kokkos::create_mirror_view(ResultData);
   Kokkos::deep_copy(ResultHost, ResultData);
   
   Real ComputedMean = ResultHost(0);
   
   // Verify
   bool Passed = approxEqual(ComputedMean, ExpectedMean);
   reportTest("SpatialMeanOp: Constant field mean", Passed);
   
   if (!Passed) {
      LOG_ERROR("  Expected: {}, Got: {}", ExpectedMean, ComputedMean);
   }
}

//------------------------------------------------------------------------------
// Test 5.1.4: SpatialStdDevOp with known values
void testSpatialStdDevOp(const MachEnv *Env,
                         const HorzMesh *Mesh,
                         const VertCoord *VCoord) {
   
   LOG_INFO("Testing SpatialStdDevOp...");
   
   // Create test field with constant value = 10.0
   // Standard deviation should be exactly 0.0
   auto valueFunc = [](I4 Cell, I4 K) -> Real {
      return 10.0;
   };
   
   createTestField("TestFieldStdDev", Mesh, VCoord, valueFunc);
   
   Real ExpectedStdDev = 0.0;
   
   // Create and compute operator
   Config EmptyConfig;
   auto StdDevOp = AnalysisOpFactory::createOp("SpatialStdDev",
                                               {"TestFieldStdDev"},
                                               EmptyConfig);
   StdDevOp->initialize(Env, Mesh, VCoord, EmptyConfig);
   
   TimeInstant TestTime;
   StdDevOp->compute(TestTime);
   
   // Get result
   auto ResultField = Field::get("TestFieldStdDev_SpatialStdDev");
   auto ResultData = ResultField->getDataArray<Array1DReal>();
   auto ResultHost = Kokkos::create_mirror_view(ResultData);
   Kokkos::deep_copy(ResultHost, ResultData);
   
   Real ComputedStdDev = ResultHost(0);
   
   // Verify
   bool Passed = approxEqual(ComputedStdDev, ExpectedStdDev);
   reportTest("SpatialStdDevOp: Constant field (zero variance)", Passed);
   
   if (!Passed) {
      LOG_ERROR("  Expected: {}, Got: {}", ExpectedStdDev, ComputedStdDev);
   }
}

//------------------------------------------------------------------------------
// Test 5.1.5: TimeMeanOp accumulation and finalization
void testTimeMeanOp(const MachEnv *Env,
                    const HorzMesh *Mesh,
                    const VertCoord *VCoord,
                    Clock *ModelClock) {
   
   LOG_INFO("Testing TimeMeanOp...");
   
   // Create test field with constant value = 5.0
   auto valueFunc = [](I4 Cell, I4 K) -> Real {
      return 5.0;
   };
   
   createTestField("TestFieldTimeMean", Mesh, VCoord, valueFunc);
   
   // Create TimeMeanOp with 3-timestep period
   Config OpConfig;
   OpConfig.set("Period", std::string("3timesteps"));
   
   auto TimeMeanOp = AnalysisOpFactory::createOp("TimeMean",
                                                  {"TestFieldTimeMean"},
                                                  OpConfig);
   TimeMeanOp->initialize(Env, Mesh, VCoord, OpConfig);
   
   // Create a period alarm for finalization
   TimeInterval ThreeSteps(3, TimeUnits::Seconds); // Assuming 1s timestep
   TimeInstant StartTime = ModelClock->getCurrentTime();
   Alarm PeriodAlarm("TestPeriodAlarm", ThreeSteps, StartTime);
   TimeMeanOp->setPeriodAlarm(&PeriodAlarm);
   
   // Accumulate over 3 timesteps
   TimeInstant Time1(0, 0, 0, 0, 0, 1); // 1 second
   TimeInstant Time2(0, 0, 0, 0, 0, 2); // 2 seconds
   TimeInstant Time3(0, 0, 0, 0, 0, 3); // 3 seconds
   
   // First accumulation
   TimeMeanOp->compute(Time1);
   
   // Second accumulation
   TimeMeanOp->compute(Time2);
   
   // Third accumulation - this should trigger finalization
   PeriodAlarm.checkRing(Time3);
   TimeMeanOp->compute(Time3);
   
   // Get result
   auto ResultField = Field::get("TestFieldTimeMean_TimeMean3timesteps");
   auto ResultData = ResultField->getDataArray<Array2DReal>();
   auto ResultHost = Kokkos::create_mirror_view(ResultData);
   Kokkos::deep_copy(ResultHost, ResultData);
   
   // Check a few values - should all be 5.0 (mean of constant field)
   Real ExpectedMean = 5.0;
   bool Passed = true;
   for (I4 Cell = 0; Cell < std::min(10, Mesh->NCellsAll); ++Cell) {
      for (I4 K = 0; K < VCoord->NVertLevels; ++K) {
         if (!approxEqual(ResultHost(Cell, K), ExpectedMean)) {
            Passed = false;
            break;
         }
      }
      if (!Passed) break;
   }
   
   reportTest("TimeMeanOp: Accumulation and finalization", Passed);
   
   if (!Passed) {
      LOG_ERROR("  Expected: {}, Got: {}", ExpectedMean, ResultHost(0, 0));
   }
}

//===----------------------------------------------------------------------===//
// Test 5.4: Factory Registration and Type Dispatch
//===----------------------------------------------------------------------===//

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
// Test 5.4.2: Verify type dispatch for different array types
void testFactoryTypeDispatch(const MachEnv *Env,
                             const HorzMesh *Mesh,
                             const VertCoord *VCoord) {
   
   LOG_INFO("Testing factory type dispatch...");
   
   // Create test fields with different types
   I4 NCells = Mesh->NCellsAll;
   I4 NVertLevels = VCoord->NVertLevels;
   
   // Test with R8 (Real/double) - already tested above
   // Test with R4 (float) if supported
   // For now, just verify that createOp doesn't crash with valid input
   
   Config EmptyConfig;
   bool Passed = true;
   
   try {
      // This should work - we already have TestFieldMax from earlier tests
      auto Op1 = AnalysisOpFactory::createOp("SpatialMax",
                                             {"TestFieldMax"},
                                             EmptyConfig);
      if (!Op1) {
         LOG_ERROR("  Failed to create SpatialMax operator");
         Passed = false;
      }
   } catch (const std::exception &e) {
      LOG_ERROR("  Exception during operator creation: {}", e.what());
      Passed = false;
   }
   
   reportTest("Factory: Type dispatch for valid field", Passed);
}

//------------------------------------------------------------------------------
// Test 5.4.3: Verify error handling for invalid operator types
void testFactoryErrorHandling() {
   
   LOG_INFO("Testing factory error handling...");
   
   Config EmptyConfig;
   bool Passed = true;
   
   // Try to create an operator that doesn't exist
   // This should either return nullptr or throw an exception
   try {
      auto InvalidOp = AnalysisOpFactory::createOp("NonExistentOperator",
                                                    {"TestFieldMax"},
                                                    EmptyConfig);
      if (InvalidOp) {
         LOG_ERROR("  Factory created operator for invalid type");
         Passed = false;
      }
   } catch (...) {
      // Expected behavior - exception thrown for invalid operator
   }
   
   reportTest("Factory: Error handling for invalid operator type", Passed);
}

//===----------------------------------------------------------------------===//
// Main test driver
//===----------------------------------------------------------------------===//

int main(int argc, char *argv[]) {
   
   int ErrCode = 0;
   
   MPI_Init(&argc, &argv);
   Kokkos::initialize();
   {
      // Initialize Omega infrastructure
      MachEnv::init(MPI_COMM_WORLD);
      MachEnv *DefEnv = MachEnv::getDefault();
      
      initLogging(DefEnv);
      
      LOG_INFO("=======================================================");
      LOG_INFO("Analysis Operator Tests (5.1 & 5.4)");
      LOG_INFO("=======================================================");
      
      Config("Omega");
      Config::readAll("omega.yml");
      
      TimeStepper::init1();
      TimeStepper *DefStepper = TimeStepper::getDefault();
      Clock *ModelClock = DefStepper->getClock();
      
      IO::init(DefEnv->getComm());
      Decomp::init();
      Field::init(ModelClock);
      Halo::init();
      HorzMesh::init();
      VertCoord::init();
      
      auto Mesh = HorzMesh::getDefault();
      auto VCoord = VertCoord::getDefault();
      
      // Register all analysis operators
      Analysis::init();
      
      LOG_INFO("");
      LOG_INFO("--- Test 5.1: Individual Operator Correctness ---");
      testSpatialMaxOp(DefEnv, Mesh, VCoord);
      testSpatialMinOp(DefEnv, Mesh, VCoord);
      testSpatialMeanOp(DefEnv, Mesh, VCoord);
      testSpatialStdDevOp(DefEnv, Mesh, VCoord);
      testTimeMeanOp(DefEnv, Mesh, VCoord, ModelClock);
      
      LOG_INFO("");
      LOG_INFO("--- Test 5.4: Factory Registration and Type Dispatch ---");
      testFactoryRegistration();
      testFactoryTypeDispatch(DefEnv, Mesh, VCoord);
      testFactoryErrorHandling();
      
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
      VertCoord::clear();
      HorzMesh::clear();
      Field::clear();
      Halo::clear();
      Decomp::clear();
      TimeStepper::clear();
      MachEnv::removeAll();
   }
   Kokkos::finalize();
   MPI_Finalize();
   
   return ErrCode;
}
