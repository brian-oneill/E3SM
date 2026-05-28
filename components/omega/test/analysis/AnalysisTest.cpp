//===-- Test driver for OMEGA Anlysis Module ---- ----------------*- C++ -*-===/
//
//===-----------------------------------------------------------------------===/

#include "Analysis.h"
#include "Decomp.h"
#include "Halo.h"
#include "HorzMesh.h"
#include "IO.h"
#include "IOStream.h"
#include "Logging.h"
#include "Pacer.h"
#include "TimeStepper.h"
#include "Tracers.h"
#include "VertAdv.h"
#include "VertCoord.h"


#include <iostream>

using namespace OMEGA;

void initAnalysisTest() {

   I4 Err;

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

   // Initialize the IO system
   IO::init(DefComm);

   // Create the default decomposition (initializes the decomposition)
   Decomp::init();

   // Initialize streams
   IOStream::init();

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

   // Initialize the default vertical advection
   VertAdv::init();

}

void finalizeAnalysisTest() {

   IOStream::finalize();
   Tracers::clear();
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

      GlobalMaxOp<Real> GlobMaxOp("PseudoThickness_max", *OmegaConfig);

      TimeInstant TStamp;
      GlobMaxOp.compute(TStamp);

      finalizeAnalysisTest();
   }

   Pacer::finalize();
   Kokkos::finalize();
   MPI_Finalize();

   return 0;

};
