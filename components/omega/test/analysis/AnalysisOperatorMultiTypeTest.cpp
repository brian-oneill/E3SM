//===-- Test 5.1: Multi-type operator correctness tests --------*- C++ -*-===//
//
// Comprehensive unit tests for Analysis operators across all supported
// array types (ranks 1D/2D/3D and scalar types I4/I8/R4/R8)
//
//===-----------------------------------------------------------------------===//

#include "Analysis.h"
#include "AnalysisOpFactory.h"
#include "Decomp.h"
#include "Field.h"
#include "Halo.h"
#include "HorzMesh.h"
#include "IO.h"
#include "IOStream.h"
#include "Logging.h"
#include "TimeStepper.h"
#include "VertCoord.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <type_traits>
#include <vector>

using namespace OMEGA;

// Test result tracking
int NumTests = 0;
int NumPassed = 0;
int NumFailed = 0;

//===----------------------------------------------------------------------===//
// Generic Helper Template Struct
//===----------------------------------------------------------------------===//

//------------------------------------------------------------------------------
// Template struct consolidating all test helper functions
template<typename ArrayType>
struct TestHelper {
   using ScalarT = typename ArrayType::non_const_value_type;
   static constexpr int Rank = ArrayType::rank;
   
   // Type-aware tolerance for floating point comparisons
   static ScalarT getTolerance() {
      if constexpr (std::is_integral_v<ScalarT>) {
         return 0;  // Exact equality for integers
      } else if constexpr (std::is_same_v<ScalarT, float>) {
         return 1.0e-6f;  // Single precision tolerance
      } else {
         return 1.0e-10;  // Double precision tolerance
      }
   }
   
   // Get dimensions based on rank
   static std::vector<I4> getDims(const HorzMesh *Mesh, const VertCoord *VCoord) {
      if constexpr (Rank == 1) {
         return {Mesh->NCellsAll};  // 1D horizontal array over cells
      } else if constexpr (Rank == 2) {
         return {Mesh->NCellsAll, VCoord->NVertLayers};
      } else if constexpr (Rank == 3) {
         return {Tracers::getNumTracers(), Mesh->NCellsAll, VCoord->NVertLayers};
      }
      return {};
   }
   
   // Get dimension names
   static std::vector<std::string> getDimNames() {
      if constexpr (Rank == 1) {
         return {"NCells"};
      } else if constexpr (Rank == 2) {
         return {"NCells", "NVertLayers"};
      } else if constexpr (Rank == 3) {
         return {"NTracers", "NCells", "NVertLayers"};
      }
      return {};
   }
   
   // Create test field for 1D arrays
   template<int R = Rank>
   static typename std::enable_if<R == 1, void>::type
   createField(const std::string &FieldName,
               const std::vector<I4> &Dims,
               std::function<ScalarT(I4)> ValueFunc) {
      
      auto DimNames = getDimNames();
      auto TestField = Field::create(
         FieldName,
         "Test field for multi-type validation",
         "m",
         "",
         -1.0e30,
         1.0e30,
         -1.0e30,
         1,
         DimNames
      );
      
      ArrayType TestData(FieldName + "_data", Dims[0]);
      TestField->attachData<ArrayType>(TestData);
      
      auto TestDataHost = Kokkos::create_mirror_view(TestData);
      for (I4 i = 0; i < Dims[0]; ++i) {
         TestDataHost(i) = ValueFunc(i);
      }
      Kokkos::deep_copy(TestData, TestDataHost);
   }
   
   // Create test field for 2D arrays
   template<int R = Rank>
   static typename std::enable_if<R == 2, void>::type
   createField(const std::string &FieldName,
               const std::vector<I4> &Dims,
               std::function<ScalarT(I4, I4)> ValueFunc) {
      
      auto DimNames = getDimNames();
      auto TestField = Field::create(
         FieldName,
         "Test field for multi-type validation",
         "m",
         "",
         -1.0e30,
         1.0e30,
         -1.0e30,
         2,
         DimNames
      );
      
      ArrayType TestData(FieldName + "_data", Dims[0], Dims[1]);
      TestField->attachData<ArrayType>(TestData);
      
      auto TestDataHost = Kokkos::create_mirror_view(TestData);
      for (I4 i = 0; i < Dims[0]; ++i) {
         for (I4 j = 0; j < Dims[1]; ++j) {
            TestDataHost(i, j) = ValueFunc(i, j);
         }
      }
      Kokkos::deep_copy(TestData, TestDataHost);
   }
   
   // Create test field for 3D arrays
   template<int R = Rank>
   static typename std::enable_if<R == 3, void>::type
   createField(const std::string &FieldName,
               const std::vector<I4> &Dims,
               std::function<ScalarT(I4, I4, I4)> ValueFunc) {
      
      auto DimNames = getDimNames();
      auto TestField = Field::create(
         FieldName,
         "Test field for multi-type validation",
         "m",
         "",
         -1.0e30,
         1.0e30,
         -1.0e30,
         3,
         DimNames
      );
      
      ArrayType TestData(FieldName + "_data", Dims[0], Dims[1], Dims[2]);
      TestField->attachData<ArrayType>(TestData);
      
      auto TestDataHost = Kokkos::create_mirror_view(TestData);
      for (I4 i = 0; i < Dims[0]; ++i) {
         for (I4 j = 0; j < Dims[1]; ++j) {
            for (I4 k = 0; k < Dims[2]; ++k) {
               TestDataHost(i, j, k) = ValueFunc(i, j, k);
            }
         }
      }
      Kokkos::deep_copy(TestData, TestDataHost);
   }
};

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
// Operator Test Templates
//===----------------------------------------------------------------------===//

//------------------------------------------------------------------------------
// Template for testing SpatialMaxOp with any array type
template<typename ArrayType>
void testSpatialMaxOp_Type(const std::string &TypeName,
                           const MachEnv *Env,
                           const HorzMesh *Mesh,
                           const VertCoord *VCoord) {
   
   using Helper = TestHelper<ArrayType>;
   using ScalarT = typename Helper::ScalarT;
   constexpr int Rank = Helper::Rank;
   
   std::vector<I4> Dims = Helper::getDims(Mesh, VCoord);
   std::string FieldName = "TestFieldMax_" + TypeName;
   
   // Create test field with values that sum indices
   if constexpr (Rank == 1) {
      Helper::createField(FieldName, Dims,
         [](I4 i) -> ScalarT {
            return static_cast<ScalarT>(i);
         });
   } else if constexpr (Rank == 2) {
      Helper::createField(FieldName, Dims,
         [](I4 i, I4 j) -> ScalarT {
            return static_cast<ScalarT>(i + j);
         });
   } else if constexpr (Rank == 3) {
      Helper::createField(FieldName, Dims,
         [](I4 i, I4 j, I4 k) -> ScalarT {
            return static_cast<ScalarT>(i + j + k);
         });
   }
   
   // Compute expected max (sum of last owned indices).
   // For 1D: operator reduces over NCellsOwned, so max of f(i)=i is NCellsOwned-1.
   // For 2D: horizontal dim restricted to NCellsOwned, max of f(i,j)=i+j is
   //   (NCellsOwned-1)+(NVertLayers-1).
   // For 3D: horizontal dim (index 1) restricted to NCellsOwned, max of
   //   f(i,j,k)=i+j+k is (NTracers-1)+(NCellsOwned-1)+(NVertLayers-1).
   ScalarT ExpectedMax = 0;
   if constexpr (Rank == 1) {
      ExpectedMax = static_cast<ScalarT>(Mesh->NCellsOwned - 1);
   } else if constexpr (Rank == 2) {
      ExpectedMax = static_cast<ScalarT>((Mesh->NCellsOwned - 1) +
                                         (VCoord->NVertLayers - 1));
   } else if constexpr (Rank == 3) {
      ExpectedMax = static_cast<ScalarT>((Tracers::getNumTracers() - 1) +
                                         (Mesh->NCellsOwned - 1) +
                                         (VCoord->NVertLayers - 1));
   }
   
   // Create and compute operator
   Config EmptyConfig;
   auto MaxOp = AnalysisOpFactory::createOp("SpatialMax", {FieldName}, EmptyConfig);
   MaxOp->initialize(Env, Mesh, VCoord, EmptyConfig);
   
   TimeInstant TestTime;
   MaxOp->compute(TestTime);
   
   // Get result
   auto ResultField = Field::get(FieldName + "_SpatialMax");
   auto ResultData = ResultField->getDataArray<Array1DReal>();
   auto ResultHost = Kokkos::create_mirror_view(ResultData);
   Kokkos::deep_copy(ResultHost, ResultData);
   
   Real ComputedMax = ResultHost(0);
   Real ExpectedMaxReal = static_cast<Real>(ExpectedMax);
   
   // Verify
   bool Passed = (std::abs(ComputedMax - ExpectedMaxReal) <= Helper::getTolerance());
   reportTest("SpatialMaxOp: " + TypeName, Passed);
   
   if (!Passed) {
      LOG_ERROR("  Expected: {}, Got: {}", ExpectedMaxReal, ComputedMax);
   }
}

//------------------------------------------------------------------------------
// Template for testing SpatialMinOp with any array type
template<typename ArrayType>
void testSpatialMinOp_Type(const std::string &TypeName,
                           const MachEnv *Env,
                           const HorzMesh *Mesh,
                           const VertCoord *VCoord) {
   
   using Helper = TestHelper<ArrayType>;
   using ScalarT = typename Helper::ScalarT;
   constexpr int Rank = Helper::Rank;
   
   std::vector<I4> Dims = Helper::getDims(Mesh, VCoord);
   std::string FieldName = "TestFieldMin_" + TypeName;
   
   // Create test field with values offset by 100
   if constexpr (Rank == 1) {
      Helper::createField(FieldName, Dims,
         [](I4 i) -> ScalarT {
            return static_cast<ScalarT>(i + 100);
         });
   } else if constexpr (Rank == 2) {
      Helper::createField(FieldName, Dims,
         [](I4 i, I4 j) -> ScalarT {
            return static_cast<ScalarT>(i + j + 100);
         });
   } else if constexpr (Rank == 3) {
      Helper::createField(FieldName, Dims,
         [](I4 i, I4 j, I4 k) -> ScalarT {
            return static_cast<ScalarT>(i + j + k + 100);
         });
   }
   
   // Expected min is 100 (all indices start at 0)
   ScalarT ExpectedMin = 100;
   
   // Create and compute operator
   Config EmptyConfig;
   auto MinOp = AnalysisOpFactory::createOp("SpatialMin", {FieldName}, EmptyConfig);
   MinOp->initialize(Env, Mesh, VCoord, EmptyConfig);
   
   TimeInstant TestTime;
   MinOp->compute(TestTime);
   
   // Get result
   auto ResultField = Field::get(FieldName + "_SpatialMin");
   auto ResultData = ResultField->getDataArray<Array1DReal>();
   auto ResultHost = Kokkos::create_mirror_view(ResultData);
   Kokkos::deep_copy(ResultHost, ResultData);
   
   Real ComputedMin = ResultHost(0);
   Real ExpectedMinReal = static_cast<Real>(ExpectedMin);
   
   // Verify
   bool Passed = (std::abs(ComputedMin - ExpectedMinReal) <= Helper::getTolerance());
   reportTest("SpatialMinOp: " + TypeName, Passed);
   
   if (!Passed) {
      LOG_ERROR("  Expected: {}, Got: {}", ExpectedMinReal, ComputedMin);
   }
}

//------------------------------------------------------------------------------
// Template for testing SpatialMeanOp with any array type
template<typename ArrayType>
void testSpatialMeanOp_Type(const std::string &TypeName,
                            const MachEnv *Env,
                            const HorzMesh *Mesh,
                            const VertCoord *VCoord) {
   
   using Helper = TestHelper<ArrayType>;
   using ScalarT = typename Helper::ScalarT;
   constexpr int Rank = Helper::Rank;
   
   std::vector<I4> Dims = Helper::getDims(Mesh, VCoord);
   std::string FieldName = "TestFieldMean_" + TypeName;
   
   // Create test field with constant value
   ScalarT ConstValue = static_cast<ScalarT>(42);
   
   if constexpr (Rank == 1) {
      Helper::createField(FieldName, Dims,
         [ConstValue](I4 i) -> ScalarT {
            return ConstValue;
         });
   } else if constexpr (Rank == 2) {
      Helper::createField(FieldName, Dims,
         [ConstValue](I4 i, I4 j) -> ScalarT {
            return ConstValue;
         });
   } else if constexpr (Rank == 3) {
      Helper::createField(FieldName, Dims,
         [ConstValue](I4 i, I4 j, I4 k) -> ScalarT {
            return ConstValue;
         });
   }
   
   // Expected mean is the constant value
   Real ExpectedMean = static_cast<Real>(ConstValue);
   
   // Create and compute operator
   Config EmptyConfig;
   auto MeanOp = AnalysisOpFactory::createOp("SpatialMean", {FieldName}, EmptyConfig);
   MeanOp->initialize(Env, Mesh, VCoord, EmptyConfig);
   
   TimeInstant TestTime;
   MeanOp->compute(TestTime);
   
   // Get result
   auto ResultField = Field::get(FieldName + "_SpatialMean");
   auto ResultData = ResultField->getDataArray<Array1DReal>();
   auto ResultHost = Kokkos::create_mirror_view(ResultData);
   Kokkos::deep_copy(ResultHost, ResultData);
   
   Real ComputedMean = ResultHost(0);
   
   // Verify
   bool Passed = (std::abs(ComputedMean - ExpectedMean) <= Helper::getTolerance());
   reportTest("SpatialMeanOp: " + TypeName, Passed);
   
   if (!Passed) {
      LOG_ERROR("  Expected: {}, Got: {}", ExpectedMean, ComputedMean);
   }
}

//------------------------------------------------------------------------------
// Template for testing SpatialStdDevOp with any array type
template<typename ArrayType>
void testSpatialStdDevOp_Type(const std::string &TypeName,
                              const MachEnv *Env,
                              const HorzMesh *Mesh,
                              const VertCoord *VCoord) {
   
   using Helper = TestHelper<ArrayType>;
   using ScalarT = typename Helper::ScalarT;
   constexpr int Rank = Helper::Rank;
   
   std::vector<I4> Dims = Helper::getDims(Mesh, VCoord);
   std::string FieldName = "TestFieldStdDev_" + TypeName;
   
   // Create test field with constant value (zero variance)
   ScalarT ConstValue = static_cast<ScalarT>(10);
   
   if constexpr (Rank == 1) {
      Helper::createField(FieldName, Dims,
         [ConstValue](I4 i) -> ScalarT {
            return ConstValue;
         });
   } else if constexpr (Rank == 2) {
      Helper::createField(FieldName, Dims,
         [ConstValue](I4 i, I4 j) -> ScalarT {
            return ConstValue;
         });
   } else if constexpr (Rank == 3) {
      Helper::createField(FieldName, Dims,
         [ConstValue](I4 i, I4 j, I4 k) -> ScalarT {
            return ConstValue;
         });
   }
   
   // Expected standard deviation is 0 (constant field)
   Real ExpectedStdDev = 0.0;
   
   // Create and compute operator
   Config EmptyConfig;
   auto StdDevOp = AnalysisOpFactory::createOp("SpatialStdDev", {FieldName}, EmptyConfig);
   StdDevOp->initialize(Env, Mesh, VCoord, EmptyConfig);
   
   TimeInstant TestTime;
   StdDevOp->compute(TestTime);
   
   // Get result
   auto ResultField = Field::get(FieldName + "_SpatialStdDev");
   auto ResultData = ResultField->getDataArray<Array1DReal>();
   auto ResultHost = Kokkos::create_mirror_view(ResultData);
   Kokkos::deep_copy(ResultHost, ResultData);
   
   Real ComputedStdDev = ResultHost(0);
   
   // Verify
   bool Passed = (std::abs(ComputedStdDev - ExpectedStdDev) <= Helper::getTolerance());
   reportTest("SpatialStdDevOp: " + TypeName, Passed);
   
   if (!Passed) {
      LOG_ERROR("  Expected: {}, Got: {}", ExpectedStdDev, ComputedStdDev);
   }
}

//------------------------------------------------------------------------------
// Template for testing TimeMeanOp with any array type
template<typename ArrayType>
void testTimeMeanOp_Type(const std::string &TypeName,
                         const MachEnv *Env,
                         const HorzMesh *Mesh,
                         const VertCoord *VCoord,
                         Clock *ModelClock) {
   
   using Helper = TestHelper<ArrayType>;
   using ScalarT = typename Helper::ScalarT;
   constexpr int Rank = Helper::Rank;
   
   std::vector<I4> Dims = Helper::getDims(Mesh, VCoord);
   std::string FieldName = "TestFieldTimeMean_" + TypeName;
   
   // Create test field with constant value
   ScalarT ConstValue = static_cast<ScalarT>(5);
   
   if constexpr (Rank == 1) {
      Helper::createField(FieldName, Dims,
         [ConstValue](I4 i) -> ScalarT {
            return ConstValue;
         });
   } else if constexpr (Rank == 2) {
      Helper::createField(FieldName, Dims,
         [ConstValue](I4 i, I4 j) -> ScalarT {
            return ConstValue;
         });
   } else if constexpr (Rank == 3) {
      Helper::createField(FieldName, Dims,
         [ConstValue](I4 i, I4 j, I4 k) -> ScalarT {
            return ConstValue;
         });
   }
   
   // Create TimeMeanOp with 3-timestep period
   Config OpConfig;
   OpConfig.set("Period", std::string("3timesteps"));
   
   auto TimeMeanOp = AnalysisOpFactory::createOp("TimeMean", {FieldName}, OpConfig);
   TimeMeanOp->initialize(Env, Mesh, VCoord, OpConfig);
   
   // Create a period alarm for finalization
   TimeInterval ThreeSteps(3, TimeUnits::Seconds);
   TimeInstant StartTime = ModelClock->getCurrentTime();
   Alarm PeriodAlarm("TestPeriodAlarm_" + TypeName, ThreeSteps, StartTime);
   TimeMeanOp->setPeriodAlarm(&PeriodAlarm);
   
   // Accumulate over 3 timesteps
   TimeInstant Time1(0, 0, 0, 0, 0, 1);
   TimeInstant Time2(0, 0, 0, 0, 0, 2);
   TimeInstant Time3(0, 0, 0, 0, 0, 3);
   
   TimeMeanOp->compute(Time1);
   TimeMeanOp->compute(Time2);
   PeriodAlarm.isRinging();
   TimeMeanOp->compute(Time3);
   
   // Get result
   std::string ResultFieldName = FieldName + "_TimeMean3timesteps";
   auto ResultField = Field::get(ResultFieldName);
   
   // Expected mean is the constant value
   Real ExpectedMean = static_cast<Real>(ConstValue);
   
   // Verify a sample of values
   bool Passed = true;
   if constexpr (Rank == 1) {
      auto ResultData = ResultField->getDataArray<Array1DReal>();
      auto ResultHost = Kokkos::create_mirror_view(ResultData);
      Kokkos::deep_copy(ResultHost, ResultData);
      
      for (I4 i = 0; i < std::min(10, Dims[0]); ++i) {
         if (std::abs(ResultHost(i) - ExpectedMean) > Helper::getTolerance()) {
            Passed = false;
            break;
         }
      }
   } else if constexpr (Rank == 2) {
      auto ResultData = ResultField->getDataArray<Array2DReal>();
      auto ResultHost = Kokkos::create_mirror_view(ResultData);
      Kokkos::deep_copy(ResultHost, ResultData);
      
      for (I4 i = 0; i < std::min(5, Dims[0]); ++i) {
         for (I4 j = 0; j < std::min(5, Dims[1]); ++j) {
            if (std::abs(ResultHost(i, j) - ExpectedMean) > Helper::getTolerance()) {
               Passed = false;
               break;
            }
         }
         if (!Passed) break;
      }
   } else if constexpr (Rank == 3) {
      auto ResultData = ResultField->getDataArray<Array3DReal>();
      auto ResultHost = Kokkos::create_mirror_view(ResultData);
      Kokkos::deep_copy(ResultHost, ResultData);
      
      for (I4 i = 0; i < std::min(3, Dims[0]); ++i) {
         for (I4 j = 0; j < std::min(3, Dims[1]); ++j) {
            for (I4 k = 0; k < std::min(3, Dims[2]); ++k) {
               if (std::abs(ResultHost(i, j, k) - ExpectedMean) > Helper::getTolerance()) {
                  Passed = false;
                  break;
               }
            }
            if (!Passed) break;
         }
         if (!Passed) break;
      }
   }
   
   reportTest("TimeMeanOp: " + TypeName, Passed);
   
   if (!Passed) {
      LOG_ERROR("  Expected: {}, Got different values", ExpectedMean);
   }
}

//===----------------------------------------------------------------------===//
// Main Test Functions
//===----------------------------------------------------------------------===//

//------------------------------------------------------------------------------
// Test SpatialMaxOp with all array types
void testSpatialMaxOp(const MachEnv *Env,
                      const HorzMesh *Mesh,
                      const VertCoord *VCoord) {
   
   LOG_INFO("Testing SpatialMaxOp with all array types...");
   
   // 1D arrays - 4 scalar types
   testSpatialMaxOp_Type<Array1DI4>("1D-I4", Env, Mesh, VCoord);
   testSpatialMaxOp_Type<Array1DI8>("1D-I8", Env, Mesh, VCoord);
   testSpatialMaxOp_Type<Array1DR4>("1D-R4", Env, Mesh, VCoord);
   testSpatialMaxOp_Type<Array1DR8>("1D-R8", Env, Mesh, VCoord);
   
   // 2D arrays - 4 scalar types
   testSpatialMaxOp_Type<Array2DI4>("2D-I4", Env, Mesh, VCoord);
   testSpatialMaxOp_Type<Array2DI8>("2D-I8", Env, Mesh, VCoord);
   testSpatialMaxOp_Type<Array2DR4>("2D-R4", Env, Mesh, VCoord);
   testSpatialMaxOp_Type<Array2DR8>("2D-R8", Env, Mesh, VCoord);
   
   // 3D arrays - 4 scalar types
   testSpatialMaxOp_Type<Array3DI4>("3D-I4", Env, Mesh, VCoord);
   testSpatialMaxOp_Type<Array3DI8>("3D-I8", Env, Mesh, VCoord);
   testSpatialMaxOp_Type<Array3DR4>("3D-R4", Env, Mesh, VCoord);
   testSpatialMaxOp_Type<Array3DR8>("3D-R8", Env, Mesh, VCoord);
}

//------------------------------------------------------------------------------
// Test SpatialMinOp with all array types
void testSpatialMinOp(const MachEnv *Env,
                      const HorzMesh *Mesh,
                      const VertCoord *VCoord) {
   
   LOG_INFO("Testing SpatialMinOp with all array types...");
   
   // 1D arrays
   testSpatialMinOp_Type<Array1DI4>("1D-I4", Env, Mesh, VCoord);
   testSpatialMinOp_Type<Array1DI8>("1D-I8", Env, Mesh, VCoord);
   testSpatialMinOp_Type<Array1DR4>("1D-R4", Env, Mesh, VCoord);
   testSpatialMinOp_Type<Array1DR8>("1D-R8", Env, Mesh, VCoord);
   
   // 2D arrays
   testSpatialMinOp_Type<Array2DI4>("2D-I4", Env, Mesh, VCoord);
   testSpatialMinOp_Type<Array2DI8>("2D-I8", Env, Mesh, VCoord);
   testSpatialMinOp_Type<Array2DR4>("2D-R4", Env, Mesh, VCoord);
   testSpatialMinOp_Type<Array2DR8>("2D-R8", Env, Mesh, VCoord);
   
   // 3D arrays
   testSpatialMinOp_Type<Array3DI4>("3D-I4", Env, Mesh, VCoord);
   testSpatialMinOp_Type<Array3DI8>("3D-I8", Env, Mesh, VCoord);
   testSpatialMinOp_Type<Array3DR4>("3D-R4", Env, Mesh, VCoord);
   testSpatialMinOp_Type<Array3DR8>("3D-R8", Env, Mesh, VCoord);
}

//------------------------------------------------------------------------------
// Test SpatialMeanOp with all array types
void testSpatialMeanOp(const MachEnv *Env,
                       const HorzMesh *Mesh,
                       const VertCoord *VCoord) {
   
   LOG_INFO("Testing SpatialMeanOp with all array types...");
   
   // 1D arrays
   testSpatialMeanOp_Type<Array1DI4>("1D-I4", Env, Mesh, VCoord);
   testSpatialMeanOp_Type<Array1DI8>("1D-I8", Env, Mesh, VCoord);
   testSpatialMeanOp_Type<Array1DR4>("1D-R4", Env, Mesh, VCoord);
   testSpatialMeanOp_Type<Array1DR8>("1D-R8", Env, Mesh, VCoord);
   
   // 2D arrays
   testSpatialMeanOp_Type<Array2DI4>("2D-I4", Env, Mesh, VCoord);
   testSpatialMeanOp_Type<Array2DI8>("2D-I8", Env, Mesh, VCoord);
   testSpatialMeanOp_Type<Array2DR4>("2D-R4", Env, Mesh, VCoord);
   testSpatialMeanOp_Type<Array2DR8>("2D-R8", Env, Mesh, VCoord);
   
   // 3D arrays
   testSpatialMeanOp_Type<Array3DI4>("3D-I4", Env, Mesh, VCoord);
   testSpatialMeanOp_Type<Array3DI8>("3D-I8", Env, Mesh, VCoord);
   testSpatialMeanOp_Type<Array3DR4>("3D-R4", Env, Mesh, VCoord);
   testSpatialMeanOp_Type<Array3DR8>("3D-R8", Env, Mesh, VCoord);
}

//------------------------------------------------------------------------------
// Test SpatialStdDevOp with all array types
void testSpatialStdDevOp(const MachEnv *Env,
                         const HorzMesh *Mesh,
                         const VertCoord *VCoord) {
   
   LOG_INFO("Testing SpatialStdDevOp with all array types...");
   
   // 1D arrays
   testSpatialStdDevOp_Type<Array1DI4>("1D-I4", Env, Mesh, VCoord);
   testSpatialStdDevOp_Type<Array1DI8>("1D-I8", Env, Mesh, VCoord);
   testSpatialStdDevOp_Type<Array1DR4>("1D-R4", Env, Mesh, VCoord);
   testSpatialStdDevOp_Type<Array1DR8>("1D-R8", Env, Mesh, VCoord);
   
   // 2D arrays
   testSpatialStdDevOp_Type<Array2DI4>("2D-I4", Env, Mesh, VCoord);
   testSpatialStdDevOp_Type<Array2DI8>("2D-I8", Env, Mesh, VCoord);
   testSpatialStdDevOp_Type<Array2DR4>("2D-R4", Env, Mesh, VCoord);
   testSpatialStdDevOp_Type<Array2DR8>("2D-R8", Env, Mesh, VCoord);
   
   // 3D arrays
   testSpatialStdDevOp_Type<Array3DI4>("3D-I4", Env, Mesh, VCoord);
   testSpatialStdDevOp_Type<Array3DI8>("3D-I8", Env, Mesh, VCoord);
   testSpatialStdDevOp_Type<Array3DR4>("3D-R4", Env, Mesh, VCoord);
   testSpatialStdDevOp_Type<Array3DR8>("3D-R8", Env, Mesh, VCoord);
}

//------------------------------------------------------------------------------
// Test TimeMeanOp with all array types
void testTimeMeanOp(const MachEnv *Env,
                    const HorzMesh *Mesh,
                    const VertCoord *VCoord,
                    Clock *ModelClock) {
   
   LOG_INFO("Testing TimeMeanOp with all array types...");
   
   // 1D arrays
   testTimeMeanOp_Type<Array1DI4>("1D-I4", Env, Mesh, VCoord, ModelClock);
   testTimeMeanOp_Type<Array1DI8>("1D-I8", Env, Mesh, VCoord, ModelClock);
   testTimeMeanOp_Type<Array1DR4>("1D-R4", Env, Mesh, VCoord, ModelClock);
   testTimeMeanOp_Type<Array1DR8>("1D-R8", Env, Mesh, VCoord, ModelClock);
   
   // 2D arrays
   testTimeMeanOp_Type<Array2DI4>("2D-I4", Env, Mesh, VCoord, ModelClock);
   testTimeMeanOp_Type<Array2DI8>("2D-I8", Env, Mesh, VCoord, ModelClock);
   testTimeMeanOp_Type<Array2DR4>("2D-R4", Env, Mesh, VCoord, ModelClock);
   testTimeMeanOp_Type<Array2DR8>("2D-R8", Env, Mesh, VCoord, ModelClock);
   
   // 3D arrays
   testTimeMeanOp_Type<Array3DI4>("3D-I4", Env, Mesh, VCoord, ModelClock);
   testTimeMeanOp_Type<Array3DI8>("3D-I8", Env, Mesh, VCoord, ModelClock);
   testTimeMeanOp_Type<Array3DR4>("3D-R4", Env, Mesh, VCoord, ModelClock);
   testTimeMeanOp_Type<Array3DR8>("3D-R8", Env, Mesh, VCoord, ModelClock);
}

//===----------------------------------------------------------------------===//
// Main test driver
//===----------------------------------------------------------------------===//

int main(int argc, char *argv[]) {
   
   int Err = 0;
   
   MPI_Init(&argc, &argv);
   Kokkos::initialize();
   Pacer::initialize(MPI_COMM_WORLD);
   Pacer::setPrefix("Omega:");
   {
      // Initialize Omega infrastructure
      MachEnv::init(MPI_COMM_WORLD);
      MachEnv *DefEnv = MachEnv::getDefault();
      
      initLogging(DefEnv);
      
      LOG_INFO("=======================================================");
      LOG_INFO("Analysis Operator Multi-Type Tests");
      LOG_INFO("Testing 5 operators × 12 array types = 60 tests");
      LOG_INFO("=======================================================");
      
      Config("Omega");
      Config::readAll("omega.yml");
      
      TimeStepper::init1();
      TimeStepper *DefStepper = TimeStepper::getDefault();
      Clock *ModelClock = DefStepper->getClock();
      
      IO::init(DefEnv->getComm());
      Decomp::init();
      IOStream::init(ModelClock);
      Field::init(ModelClock);
      Err = Halo::init();
      if (Err != 0)
         ABORT_ERROR("VertAdvTest: error initializing default halo");
      HorzMesh::init();
      VertCoord::init();
      Tracers::init();
      AuxiliaryState::init();
      Eos::init();
      PressureGrad::init();
      Tendencies::init();
      VertAdv::init();
      TimeStepper::init2();
      Err = OceanState::init();
      if (Err != 0)
         ABORT_ERROR("ocnInit: Error initializing default state");
      
      auto Mesh = HorzMesh::getDefault();
      auto VCoord = VertCoord::getDefault();
      
      // Register all analysis operators
      Analysis::init();
      
      LOG_INFO("");
      LOG_INFO("--- Testing SpatialMaxOp (12 array types) ---");
      testSpatialMaxOp(DefEnv, Mesh, VCoord);
      
      LOG_INFO("");
      LOG_INFO("--- Testing SpatialMinOp (12 array types) ---");
      testSpatialMinOp(DefEnv, Mesh, VCoord);
      
      LOG_INFO("");
      LOG_INFO("--- Testing SpatialMeanOp (12 array types) ---");
      testSpatialMeanOp(DefEnv, Mesh, VCoord);
      
      LOG_INFO("");
      LOG_INFO("--- Testing SpatialStdDevOp (12 array types) ---");
      testSpatialStdDevOp(DefEnv, Mesh, VCoord);
      
      LOG_INFO("");
      LOG_INFO("--- Testing TimeMeanOp (12 array types) ---");
      testTimeMeanOp(DefEnv, Mesh, VCoord, ModelClock);
      
      // Report summary
      LOG_INFO("");
      LOG_INFO("=======================================================");
      LOG_INFO("Test Summary:");
      LOG_INFO("  Total tests: {}", NumTests);
      LOG_INFO("  Passed: {}", NumPassed);
      LOG_INFO("  Failed: {}", NumFailed);
      LOG_INFO("=======================================================");
      
      if (NumFailed > 0) {
         Err = 1;
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
      Halo::clear();
      Decomp::clear();
      MachEnv::removeAll();
   }
   Pacer::finalize();
   Kokkos::finalize();
   MPI_Finalize();
   
   return Err;
}
