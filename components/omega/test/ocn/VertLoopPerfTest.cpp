
#include "HorzMesh.h"
#include "Config.h"
#include "DataTypes.h"
#include "Decomp.h"
#include "Dimension.h"
#include "Halo.h"
#include "IO.h"
#include "Logging.h"
#include "MachEnv.h"
#include "OmegaKokkos.h"
#include "Pacer.h"
#include "mpi.h"

#include <iostream>

using namespace OMEGA;

class VerticalMesh {

 public:
   HorzMesh *MyHorzMesh;
   I4 NVertLevels;

   Real XMaxCell, XMaxEdge, XMaxVertex;

   // Max Level
   Array1DI4 MaxLevelCell;
   HostArray1DI4 MaxLevelCellH;
   
   Array1DI4 MaxLevelEdgeTop;
   HostArray1DI4 MaxLevelEdgeTopH;

   Array1DI4 MaxLevelEdgeBot;
   HostArray1DI4 MaxLevelEdgeBotH;                                                         
   Array1DI4 MaxLevelVertexTop;
   HostArray1DI4 MaxLevelVertexTopH;

   Array1DI4 MaxLevelVertexBot;
   HostArray1DI4 MaxLevelVertexBotH;

   Array2DReal CellMask;
   HostArray2DReal CellMaskH;

   Array2DReal EdgeMask;
   HostArray2DReal EdgeMaskH;

   Array2DReal VertexMask;
   HostArray2DReal VertexMaskH;



   VerticalMesh(HorzMesh *InHorzMesh) {
      MyHorzMesh = InHorzMesh;
      NVertLevels = MyHorzMesh->NVertLevels;

      MaxLevelCell = Array1DI4("MaxLevelCell", MyHorzMesh->NCellsSize);
      MaxLevelEdgeTop = Array1DI4("MaxLevelEdgeTop", MyHorzMesh->NEdgesSize);
      MaxLevelEdgeBot = Array1DI4("MaxLevelEdgeBot", MyHorzMesh->NEdgesSize);   
      MaxLevelVertexTop = Array1DI4("MaxLevelVertexTop", MyHorzMesh->NVerticesSize);
      MaxLevelVertexBot = Array1DI4("MaxLevelVertexBot", MyHorzMesh->NVerticesSize);

      CellMask = Array2DReal("CellMask", MyHorzMesh->NCellsSize, NVertLevels);
      EdgeMask = Array2DReal("EdgeMask", MyHorzMesh->NEdgesSize, NVertLevels);
      VertexMask = Array2DReal("VertexMask", MyHorzMesh->NVerticesSize, NVertLevels);


   }

   int setFloor(int NFloor) {

      OMEGA_SCOPE(LocMaxLevelCell, MaxLevelCell);
      OMEGA_SCOPE(LocCellMask, CellMask);

      parallelFor(
          {MyHorzMesh->NCellsSize}, KOKKOS_LAMBDA(int ICell) {
              LocMaxLevelCell(ICell) = NFloor;
      });
      parallelFor(
          {NCellsSize}, KOKKOS_LAMBDA(int ICell, int K) {
             LocCellMask(ICell, K) = (K < LocMaxLevelCell(ICell) ? 1. : 0.);
      });



   }

};

int initTest() {

   I4 Err = 0;

   MachEnv::init(MPI_COMM_WORLD);
   MachEnv *DefEnv  = MachEnv::getDefault();
   MPI_Comm DefComm = DefEnv->getComm();

   // Initialize logging
   initLogging(DefEnv);

   // Open config file
   Config("Omega");
   Err = Config::readAll("omega.yml");
   if (Err != 0) {
      LOG_CRITICAL("VertLoopPerfTest: Error reading config file");
      return Err;
   }

   I4 IOErr = IO::init(DefComm);
   if (IOErr != 0) {
      Err++;
      LOG_ERROR("VertLoopPerfTest: error initializing parallel IO");
   }

   int DecompErr = Decomp::init();
   if (DecompErr != 0) {
      Err++;
      LOG_ERROR("VertLoopPerfTest: error initializing default decomposition");
   }

   int HaloErr = Halo::init();
   if (HaloErr != 0) {
      Err++;
      LOG_ERROR("VertLoopPerfTest: error initializing default halo");
   }

   int MeshErr = HorzMesh::init();
   if (MeshErr != 0) {
      Err++;
      LOG_ERROR("VertLoopPerfTest: error initializing default mesh");
   }

   return Err;

}

int main(int argc, char *argv[]) {

   int RetVal = 0;

   MPI_Init(&argc, &argv);
   Kokkos::initialize();
   Pacer::initialize(MPI_COMM_WORLD);
   Pacer::setPrefix("Omega:");
   {

      RetVal += initTest();
   
      HorzMesh *DefHorzMesh = HorzMesh::getDefault();
   
      VerticalMesh VertMesh(DefHorzMesh);

      HorzMesh::clear();
      Dimension::clear();
      Halo::clear();
      Decomp::clear();
      MachEnv::removeAll();
   }
   Kokkos::finalize();
   MPI_Finalize();


   return RetVal;
} // end of main
