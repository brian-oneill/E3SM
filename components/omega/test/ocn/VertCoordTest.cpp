//===-- Test driver for OMEGA Vertical Coordinate ----------------*- C++ -*-===/
//
/// \file
/// \brief Test driver for OMEGA VertCoord class
///
///
//
//===-----------------------------------------------------------------------===/

#include "VertCoord.h"
#include "DataTypes.h"
#include "Decomp.h"
#include "Dimension.h"
#include "Error.h"
#include "Halo.h"
#include "HorzMesh.h"
#include "IO.h"
#include "Logging.h"
#include "MachEnv.h"
#include "OmegaKokkos.h"
#include "Pacer.h"
#include "mpi.h"

#include <iostream>

using namespace OMEGA;

int initVertCoordTest() {

   int Err = 0;

   MachEnv::init(MPI_COMM_WORLD);
   MachEnv *DefEnv  = MachEnv::getDefault();
   MPI_Comm DefComm = DefEnv->getComm();

   // Initialize the Logging system
   initLogging(DefEnv);

   // Open config file
   Config("Omega");
   Config::readAll("omega.yml");

   // Initialize the IO system
   Err = IO::init(DefComm);
   if (Err != 0)
      LOG_ERROR("HorzMeshTest: error initializing parallel IO");

   // Create the default decomposition (initializes the decomposition)
   Decomp::init();

   // Initialize the default halo
   Err = Halo::init();
   if (Err != 0)
      LOG_ERROR("HorzMeshTest: error initializing default halo");

   // Initialize the default mesh
   HorzMesh::init();

   VertCoord::init();

   return Err;
} // end initVertCoordTest

//------------------------------------------------------------------------------
// The test driver for VertCoord test
//
int main(int argc, char *argv[]) {

   int RetVal = 0;

   // Initialize the global MPI environment
   MPI_Init(&argc, &argv);
   Kokkos::initialize();
   Pacer::initialize(MPI_COMM_WORLD);
   Pacer::setPrefix("Omega:");
   {
      int Err = initVertCoordTest();
      if (Err != 0)
         LOG_CRITICAL("VertCoordTest: Error initializing");

      auto *DefVertCoord = VertCoord::getDefault();
      auto *DefMesh      = HorzMesh::getDefault();

      //      for (int I = 0; I < DefMesh->NCellsSize; ++I) {
      //         std::cout << "   " << I << "   " <<
      //         DefVertCoord->MaxLevelCell(I) << "   "; std::cout <<
      //         DefVertCoord->MinLevelCell(I) << std::endl;
      //      }

      //      for (int I = 0; I < DefMesh->NEdgesSize; ++I) {
      //         std::cout << "   " << I << "   " <<
      //         DefVertCoord->MaxLevelEdgeTop(I) << "   "; std::cout <<
      //         DefVertCoord->MaxLevelEdgeBot(I) << "   "; std::cout <<
      //         DefVertCoord->MinLevelEdgeTop(I) << "   "; std::cout <<
      //         DefVertCoord->MinLevelEdgeBot(I) << std::endl;
      //      }

      //      for (int I = 0; I < DefMesh->NVerticesSize; ++I) {
      //         std::cout << "   " << I << "   " <<
      //         DefVertCoord->MaxLevelVertexTop(I)
      //                   << "   ";
      //         std::cout << DefVertCoord->MaxLevelVertexBot(I) << "   ";
      //         std::cout << DefVertCoord->MinLevelVertexTop(I) << "   ";
      //         std::cout << DefVertCoord->MinLevelVertexBot(I) << std::endl;
      //      }

      // Finalize Omega objects
      VertCoord::clear();
      HorzMesh::clear();
      Dimension::clear();
      Halo::clear();
      Decomp::clear();
      MachEnv::removeAll();
   }
   Kokkos::finalize();
   MPI_Finalize();

   if (RetVal >= 256)
      RetVal = 255;

   return RetVal;

} // end of main
//===-----------------------------------------------------------------------===/
