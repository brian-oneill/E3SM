//===-- Test driver for OMEGA Anlysis Module ---- ----------------*- C++ -*-===/
//
//===-----------------------------------------------------------------------===/

#include "Analysis.h"
#include "AuxiliaryState.h"
#include "Decomp.h"
#include "Field.h"
#include "Halo.h"
#include "HorzMesh.h"
#include "IO.h"
#include "IOStream.h"
#include "Logging.h"
#include "OceanState.h"
#include "Pacer.h"
#include "PGrad.h"
#include "Tendencies.h"
#include "TimeStepper.h"
#include "Tracers.h"
#include "VertAdv.h"
#include "VertCoord.h"


#include <iostream>

using namespace OMEGA;

void initAnalysisTest() {

   I4 Err;
   Error Err1;

   MachEnv::init(MPI_COMM_WORLD);
   MachEnv *DefEnv  = MachEnv::getDefault();
   MPI_Comm DefComm = DefEnv->getComm();

   // Initialize the Logging system
   initLogging(DefEnv);

   // Open config file
   Config("Omega");
   Config::readAll("omega.yml");

   // First step of time stepper initialization needed for IOstream
   TimeStepper::init1();

   TimeStepper *DefStepper = TimeStepper::getDefault();
   Clock *ModelClock       = DefStepper->getClock();

   // Initialize the IO system
   IO::init(DefComm);

   // Create the default decomposition (initializes the decomposition)
   Decomp::init();

   // Initialize streams
   IOStream::init(ModelClock);

   // Initialize Field infrastructure
   Field::init(ModelClock);

   // Initialize the default halo
   Err = Halo::init();
   if (Err != 0)
      ABORT_ERROR("VertAdvTest: error initializing default halo");

   // Initialize the default mesh
   HorzMesh::init();

   // Initialize the default vertical coordinate
   VertCoord::init();

   // Initialize tracers
   Tracers::init();

   // Initialize Aux State variables
   AuxiliaryState::init();

   // Initialize pressure gradient
   PressureGrad::init();

   // Create tendencies
   Tendencies::init();

   // Initialize the default vertical advection
   VertAdv::init();

   // Finish time stepper initialization
   TimeStepper::init2();

   // Create a default ocean state
   Err = OceanState::init();
   if (Err != 0)
      ABORT_ERROR("ocnInit: Error initializing default state");

   // Now that all fields have been defined, validate all the streams
   // contents
   bool StreamsValid = IOStream::validateAll();
   if (!StreamsValid)
      ABORT_ERROR("ocnInit: Error validating IO Streams");

   // Read the state variables from the initial state stream
   Metadata ReqMeta; // no global metadata needed for init state read
   Err1 = IOStream::read("InitialState", ModelClock, ReqMeta);
   CHECK_ERROR_ABORT(Err1, "ocnInit: Error reading initial state from stream");


   OceanState *DefState = OceanState::getDefault();
   DefState->exchangeHalo(0);

   AuxiliaryState *DefAuxState = AuxiliaryState::getDefault();
   DefAuxState->exchangeHalo();

//   auto DefMesh = HorzMesh::getDefault();
//   auto DefVCoord = VertCoord::getDefault();
//   auto PsThick = DefState->getPseudoThickness(0);
//   for (int I = DefMesh->NCellsOwned; I < DefMesh->NCellsAll; ++I) {
//      for (int K = 0; K < DefVCoord->NVertLayers; ++K) {
//         std::cout << I << " " << K << " " << PsThick(I, K) << std::endl;
//      }
//   }


}

void finalizeAnalysisTest() {

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



int main(int argc, char *argv[]) {

   // Initialize error code
   Error ErrAll;

   MPI_Init(&argc, &argv);
   Kokkos::initialize();
   Pacer::initialize(MPI_COMM_WORLD);
   Pacer::setPrefix("Omega:");
   {
      initAnalysisTest();

      auto DefMesh = HorzMesh::getDefault();
      auto DefVCoord = VertCoord::getDefault();
      auto DefEnv  = MachEnv::getDefault();

      auto OmegaConfig = Config::getOmegaConfig();

      Analysis::init();

      TimeInstant TStamp;

/*

//      SpatialMaxOp<Real> GlobMaxOp("PseudoThickness", *OmegaConfig);
//
//      GlobMaxOp.initialize(OmegaConfig, DefEnv, DefMesh, DefVCoord);
//
//      GlobMaxOp.compute(TStamp);
//
//      std::cout << GlobMaxOp.getVal() << std::endl;
//      std::cout << AnalysisOpFactory::hasOperator("spatial_max_R8") << " " << AnalysisOpFactory::hasOperator("spatial_max") << std::endl;

//      auto GlobMaxOp = AnalysisOpFactory::createOp("spatial_max_2DR8", "PseudoThickness", *OmegaConfig);


      auto GlobMaxOp = AnalysisOpFactory::createOp("spatial_max", "PseudoThickness", "PseudoThickness", *OmegaConfig);

      GlobMaxOp->initialize(OmegaConfig, DefEnv, DefMesh, DefVCoord);

      GlobMaxOp->compute(TStamp);

      auto GlobMaxArray =  Field::getFieldDataArray<Array1DReal>("PseudoThickness_spatial_max");

      std::cout << GlobMaxArray(0) << std::endl;


//      std::cout << GlobMaxOp->getVal() << std::endl;

//      SpatialMinOp<Real> GlobMinOp("PseudoThickness", *OmegaConfig);
//
//      GlobMinOp.initialize(OmegaConfig, DefEnv, DefMesh, DefVCoord);
//
//      GlobMinOp.compute(TStamp);
//
//      std::cout << GlobMinOp.getVal() << std::endl;


      auto GlobMinOp = AnalysisOpFactory::createOp("spatial_min", "PseudoThickness", "PseudoThickness", *OmegaConfig);

      GlobMinOp->initialize(OmegaConfig, DefEnv, DefMesh, DefVCoord);

      GlobMinOp->compute(TStamp);

      auto GlobMinArray =  Field::getFieldDataArray<Array1DReal>("PseudoThickness_spatial_min");

      std::cout << GlobMinArray(0) << std::endl;


//
//      SpatialMeanOp<Real> GlobMeanOp("PseudoThickness", *OmegaConfig);
//
//      GlobMeanOp.initialize(OmegaConfig, DefEnv, DefMesh, DefVCoord);
//
//      GlobMeanOp.compute(TStamp);
//
//      std::cout << GlobMeanOp.getVal() << std::endl;
//
//



      auto GlobMeanOp = AnalysisOpFactory::createOp("spatial_mean", "PseudoThickness", "PseudoThickness", *OmegaConfig);

      GlobMeanOp->initialize(OmegaConfig, DefEnv, DefMesh, DefVCoord);

      GlobMeanOp->compute(TStamp);

      auto GlobMeanArray =  Field::getFieldDataArray<Array1DReal>("PseudoThickness_spatial_mean");

      std::cout << GlobMeanArray(0) << std::endl;


      auto SigmaOp = AnalysisOpFactory::createOp("standard_dev", "PseudoThickness", "PseudoThickness", *OmegaConfig);

      SigmaOp->initialize(OmegaConfig, DefEnv, DefMesh, DefVCoord);

      SigmaOp->compute(TStamp);

      auto SigmaArray =  Field::getFieldDataArray<Array1DReal>("PseudoThickness_stddev");

      std::cout << SigmaArray(0) << std::endl;
*/


      finalizeAnalysisTest();

   }

   Pacer::finalize();
   Kokkos::finalize();
   MPI_Finalize();

   return 0;

};
