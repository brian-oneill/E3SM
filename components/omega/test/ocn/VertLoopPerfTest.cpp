
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
#include <unistd.h>
#include <string>

using namespace OMEGA;

//constexpr int OMEGA_TEAMSIZE = 64;
//constexpr int OMEGA_TEAMSIZE = 32;
//constexpr int OMEGA_TEAMSIZE = 96;
//constexpr int OMEGA_TEAMSIZE = 48;
//constexpr int OMEGA_TEAMSIZE = 100;
constexpr int OMEGA_TEAMSIZE = 50;

constexpr int NNTracers = 5;

class DivergenceWMask {
 public:

   KOKKOS_FUNCTION void operator()(const Array2DReal &DivCell, int ICell,
                                   int KChunk,
                                   const Array2DReal &VecEdge) const {
      const int KStart       = KChunk * VecLength;
      const Real InvAreaCell = 1._Real / AreaCell(ICell);

      Real DivCellTmp[VecLength] = {0};

      for (int J = 0; J < NEdgesOnCell(ICell); ++J) {
         const int JEdge = EdgesOnCell(ICell, J);
         for (int KVec = 0; KVec < VecLength; ++KVec) {
            const int K = KStart + KVec;
            DivCellTmp[KVec] -= DvEdge(JEdge) * EdgeSignOnCell(ICell, J) *
                                VecEdge(JEdge, K) * InvAreaCell;
         }
      }

      for (int KVec = 0; KVec < VecLength; ++KVec) {
         const int K       = KStart + KVec;
         DivCell(ICell, K) = CellMask(ICell, K) * DivCellTmp[KVec];
      }
   }

 private:
   Array1DI4 NEdgesOnCell;
   Array2DI4 EdgesOnCell;
   Array1DReal DvEdge;
   Array1DReal AreaCell;
   Array2DReal EdgeSignOnCell;
   Array2DReal CellMask;

 public:
   DivergenceWMask(HorzMesh const *Mesh, Array2DReal InCellMask)
    : NEdgesOnCell(Mesh->NEdgesOnCell), EdgesOnCell(Mesh->EdgesOnCell),
      DvEdge(Mesh->DvEdge), AreaCell(Mesh->AreaCell),
      EdgeSignOnCell(Mesh->EdgeSignOnCell), CellMask(InCellMask) {}
};

class DivergenceWMask2 {
 public:

   KOKKOS_FUNCTION void operator()(const Array2DReal &DivCell, int ICell,
                                   int KChunk,
                                   const Array2DReal &VecEdge) const {
      const int KStart       = KChunk * VecLength;
      const Real InvAreaCell = 1._Real / AreaCell(ICell);

      if (CellMask(ICell, KChunk)) {

         Real DivCellTmp[VecLength] = {0};

         for (int J = 0; J < NEdgesOnCell(ICell); ++J) {
            const int JEdge = EdgesOnCell(ICell, J);
            for (int KVec = 0; KVec < VecLength; ++KVec) {
               const int K = KStart + KVec;
               DivCellTmp[KVec] -= DvEdge(JEdge) * EdgeSignOnCell(ICell, J) *
                                   VecEdge(JEdge, K) * InvAreaCell;
            }
         }

         for (int KVec = 0; KVec < VecLength; ++KVec) {
            const int K       = KStart + KVec;
            DivCell(ICell, K) = DivCellTmp[KVec];
         }
      }
   }

 private:
   Array1DI4 NEdgesOnCell;
   Array2DI4 EdgesOnCell;
   Array1DReal DvEdge;
   Array1DReal AreaCell;
   Array2DReal EdgeSignOnCell;
   Array2DReal CellMask;

 public:
   DivergenceWMask2(HorzMesh const *Mesh, Array2DReal InCellMask)
    : NEdgesOnCell(Mesh->NEdgesOnCell), EdgesOnCell(Mesh->EdgesOnCell),
      DvEdge(Mesh->DvEdge), AreaCell(Mesh->AreaCell),
      EdgeSignOnCell(Mesh->EdgeSignOnCell), CellMask(InCellMask) {}
};

class DivergenceNoMask {
 public:

   KOKKOS_FUNCTION void operator()(const Array2DReal &DivCell, int ICell,
                                   int KChunk,
                                   const Array2DReal &VecEdge) const {
      const int KStart       = KChunk * VecLength;
      const Real InvAreaCell = 1._Real / AreaCell(ICell);

      Real DivCellTmp[VecLength] = {0};

      for (int J = 0; J < NEdgesOnCell(ICell); ++J) {
         const int JEdge = EdgesOnCell(ICell, J);
         for (int KVec = 0; KVec < VecLength; ++KVec) {
            const int K = KStart + KVec;
            DivCellTmp[KVec] -= DvEdge(JEdge) * EdgeSignOnCell(ICell, J) *
                                VecEdge(JEdge, K) * InvAreaCell;
         }
      }

      for (int KVec = 0; KVec < VecLength; ++KVec) {
         const int K       = KStart + KVec;
         DivCell(ICell, K) = DivCellTmp[KVec];
      }
   }

 private:
   Array1DI4 NEdgesOnCell;
   Array2DI4 EdgesOnCell;
   Array1DReal DvEdge;
   Array1DReal AreaCell;
   Array2DReal EdgeSignOnCell;

 public:
   DivergenceNoMask(HorzMesh const *Mesh)
    : NEdgesOnCell(Mesh->NEdgesOnCell), EdgesOnCell(Mesh->EdgesOnCell),
      DvEdge(Mesh->DvEdge), AreaCell(Mesh->AreaCell),
      EdgeSignOnCell(Mesh->EdgeSignOnCell) {}

};

class GradientWMask {
 public:

   KOKKOS_FUNCTION void operator()(const Array2DReal &GradEdge, int IEdge,
                                   int KChunk,
                                   const Array2DReal &ScalarCell) const {
      const int KStart     = KChunk * VecLength;
      const Real InvDcEdge = 1._Real / DcEdge(IEdge);
      const auto JCell0    = CellsOnEdge(IEdge, 0);
      const auto JCell1    = CellsOnEdge(IEdge, 1);

      for (int KVec = 0; KVec < VecLength; ++KVec) {
         const int K = KStart + KVec;
         GradEdge(IEdge, K) = EdgeMask(IEdge, K) *
             InvDcEdge * (ScalarCell(JCell1, K) - ScalarCell(JCell0, K));
      }
   }

 private:
   Array2DI4 CellsOnEdge;
   Array1DReal DcEdge;
   Array2DReal EdgeMask;

 public:
   GradientWMask(HorzMesh const *Mesh, Array2DReal InEdgeMask)
    : CellsOnEdge(Mesh->CellsOnEdge), DcEdge(Mesh->DcEdge), EdgeMask(InEdgeMask) {}

};

class GradientWMask2 {
 public:

   KOKKOS_FUNCTION void operator()(const Array2DReal &GradEdge, int IEdge,
                                   int KChunk,
                                   const Array2DReal &ScalarCell) const {
      const int KStart     = KChunk * VecLength;
      const Real InvDcEdge = 1._Real / DcEdge(IEdge);
      const auto JCell0    = CellsOnEdge(IEdge, 0);
      const auto JCell1    = CellsOnEdge(IEdge, 1);

      if ( EdgeMask(IEdge, KChunk) ) {
         for (int KVec = 0; KVec < VecLength; ++KVec) {
            const int K = KStart + KVec;
            GradEdge(IEdge, K) =
                InvDcEdge * (ScalarCell(JCell1, K) - ScalarCell(JCell0, K));
         }
      }
   }

 private:
   Array2DI4 CellsOnEdge;
   Array1DReal DcEdge;
   Array2DReal EdgeMask;

 public:
   GradientWMask2(HorzMesh const *Mesh, Array2DReal InEdgeMask)
    : CellsOnEdge(Mesh->CellsOnEdge), DcEdge(Mesh->DcEdge), EdgeMask(InEdgeMask) {}

};

class GradientNoMask {
 public:

   KOKKOS_FUNCTION void operator()(const Array2DReal &GradEdge, int IEdge,
                                   int KChunk,
                                   const Array2DReal &ScalarCell) const {
      const int KStart     = KChunk * VecLength;
      const Real InvDcEdge = 1._Real / DcEdge(IEdge);
      const auto JCell0    = CellsOnEdge(IEdge, 0);
      const auto JCell1    = CellsOnEdge(IEdge, 1);

      for (int KVec = 0; KVec < VecLength; ++KVec) {
         const int K = KStart + KVec;
         GradEdge(IEdge, K) =
             InvDcEdge * (ScalarCell(JCell1, K) - ScalarCell(JCell0, K));
      }
   }

 private:
   Array2DI4 CellsOnEdge;
   Array1DReal DcEdge;

 public:
   GradientNoMask(HorzMesh const *Mesh)
    : CellsOnEdge(Mesh->CellsOnEdge), DcEdge(Mesh->DcEdge) {}
};

class CurlWMask {
 public:

   KOKKOS_FUNCTION void operator()(const Array2DReal &CurlVertex, int IVertex,
                                   int KChunk,
                                   const Array2DReal &VecEdge) const {
      const int KStart           = KChunk * VecLength;
      const Real InvAreaTriangle = 1._Real / AreaTriangle(IVertex);

      Real CurlVertexTmp[VecLength] = {0};

      for (int J = 0; J < VertexDegree; ++J) {
         const int JEdge = EdgesOnVertex(IVertex, J);
         for (int KVec = 0; KVec < VecLength; ++KVec) {
            const int K = KStart + KVec;
            CurlVertexTmp[KVec] += DcEdge(JEdge) *
                                   EdgeSignOnVertex(IVertex, J) *
                                   VecEdge(JEdge, K) * InvAreaTriangle;
         }
      }

      for (int KVec = 0; KVec < VecLength; ++KVec) {
         const int K            = KStart + KVec;
         CurlVertex(IVertex, K) = VertexMask(IVertex, K) * CurlVertexTmp[KVec];
      }
   }

 private:
   I4 VertexDegree;
   Array2DI4 EdgesOnVertex;
   Array1DReal DcEdge;
   Array1DReal AreaTriangle;
   Array2DReal EdgeSignOnVertex;
   Array2DReal VertexMask;

 public:
   CurlWMask(HorzMesh const *Mesh, Array2DReal InVertexMask)
    : VertexDegree(Mesh->VertexDegree), EdgesOnVertex(Mesh->EdgesOnVertex),
      DcEdge(Mesh->DcEdge), AreaTriangle(Mesh->AreaTriangle),
      EdgeSignOnVertex(Mesh->EdgeSignOnVertex), VertexMask(InVertexMask) {}

};

class CurlWMask2 {
 public:

   KOKKOS_FUNCTION void operator()(const Array2DReal &CurlVertex, int IVertex,
                                   int KChunk,
                                   const Array2DReal &VecEdge) const {
      const int KStart           = KChunk * VecLength;
      const Real InvAreaTriangle = 1._Real / AreaTriangle(IVertex);

      if (VertexMask(IVertex, KChunk)) {

         Real CurlVertexTmp[VecLength] = {0};

         for (int J = 0; J < VertexDegree; ++J) {
            const int JEdge = EdgesOnVertex(IVertex, J);
            for (int KVec = 0; KVec < VecLength; ++KVec) {
               const int K = KStart + KVec;
               CurlVertexTmp[KVec] += DcEdge(JEdge) *
                                      EdgeSignOnVertex(IVertex, J) *
                                      VecEdge(JEdge, K) * InvAreaTriangle;
            }
         }

         for (int KVec = 0; KVec < VecLength; ++KVec) {
            const int K            = KStart + KVec;
            CurlVertex(IVertex, K) = CurlVertexTmp[KVec];
         }
      }
   }

 private:
   I4 VertexDegree;
   Array2DI4 EdgesOnVertex;
   Array1DReal DcEdge;
   Array1DReal AreaTriangle;
   Array2DReal EdgeSignOnVertex;
   Array2DReal VertexMask;

 public:
   CurlWMask2(HorzMesh const *Mesh, Array2DReal InVertexMask)
    : VertexDegree(Mesh->VertexDegree), EdgesOnVertex(Mesh->EdgesOnVertex),
      DcEdge(Mesh->DcEdge), AreaTriangle(Mesh->AreaTriangle),
      EdgeSignOnVertex(Mesh->EdgeSignOnVertex), VertexMask(InVertexMask) {}

};

class CurlNoMask {
 public:

   KOKKOS_FUNCTION void operator()(const Array2DReal &CurlVertex, int IVertex,
                                   int KChunk,
                                   const Array2DReal &VecEdge) const {
      const int KStart           = KChunk * VecLength;
      const Real InvAreaTriangle = 1._Real / AreaTriangle(IVertex);

      Real CurlVertexTmp[VecLength] = {0};

      for (int J = 0; J < VertexDegree; ++J) {
         const int JEdge = EdgesOnVertex(IVertex, J);
         for (int KVec = 0; KVec < VecLength; ++KVec) {
            const int K = KStart + KVec;
            CurlVertexTmp[KVec] += DcEdge(JEdge) *
                                   EdgeSignOnVertex(IVertex, J) *
                                   VecEdge(JEdge, K) * InvAreaTriangle;
         }
      }

      for (int KVec = 0; KVec < VecLength; ++KVec) {
         const int K            = KStart + KVec;
         CurlVertex(IVertex, K) = CurlVertexTmp[KVec];
      }
   }

 private:
   I4 VertexDegree;
   Array2DI4 EdgesOnVertex;
   Array1DReal DcEdge;
   Array1DReal AreaTriangle;
   Array2DReal EdgeSignOnVertex;

 public:
   CurlNoMask(HorzMesh const *Mesh)
    : VertexDegree(Mesh->VertexDegree), EdgesOnVertex(Mesh->EdgesOnVertex),
      DcEdge(Mesh->DcEdge), AreaTriangle(Mesh->AreaTriangle),
      EdgeSignOnVertex(Mesh->EdgeSignOnVertex) {}
};



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

      I4 Retval = 0;

      OMEGA_SCOPE(LocMaxLevelCell, MaxLevelCell);

      parallelFor(
          {MyHorzMesh->NCellsSize}, KOKKOS_LAMBDA(int ICell) {
              LocMaxLevelCell(ICell) = NFloor;
      });
      OMEGA_SCOPE(LocCellMask, CellMask);
      parallelFor(
          {MyHorzMesh->NCellsSize, NVertLevels}, KOKKOS_LAMBDA(int ICell, int K) {
             LocCellMask(ICell, K) = (K < LocMaxLevelCell(ICell) ? 1. : 0.);
      });

      OMEGA_SCOPE(LocMaxLevelEdgeTop, MaxLevelEdgeTop);
      OMEGA_SCOPE(LocMaxLevelEdgeBot, MaxLevelEdgeBot);
      OMEGA_SCOPE(LocCellsOnEdge, MyHorzMesh->CellsOnEdge);

      parallelFor(
          {MyHorzMesh->NEdgesSize}, KOKKOS_LAMBDA(int IEdge) {
             LocMaxLevelEdgeTop(IEdge) = NFloor;
             LocMaxLevelEdgeBot(IEdge) = NFloor;
//             LocMaxLevelEdgeTop(IEdge) =
//                 std::min(LocMaxLevelCell(LocCellsOnEdge(IEdge,0)),
//                          LocMaxLevelCell(LocCellsOnEdge(IEdge,1)));
//             LocMaxLevelEdgeBot(IEdge) =
//                 std::max(LocMaxLevelCell(LocCellsOnEdge(IEdge,0)),
//                          LocMaxLevelCell(LocCellsOnEdge(IEdge,1)));
      });

      OMEGA_SCOPE(LocEdgeMask, EdgeMask);
      parallelFor(
          {MyHorzMesh->NEdgesSize, NVertLevels}, KOKKOS_LAMBDA(int IEdge, int K) {
             LocEdgeMask(IEdge, K) = (K < LocMaxLevelEdgeTop(IEdge) ? 1. : 0.);
          });

      OMEGA_SCOPE(LocMaxLevelVertexTop, MaxLevelVertexTop);
      OMEGA_SCOPE(LocMaxLevelVertexBot, MaxLevelVertexBot);
      OMEGA_SCOPE(LocCellsOnVertex, MyHorzMesh->CellsOnVertex);
      OMEGA_SCOPE(LocVertexDegree, MyHorzMesh->VertexDegree);

      parallelFor(
          {MyHorzMesh->NVerticesSize}, KOKKOS_LAMBDA(int IVertex) {
             LocMaxLevelVertexTop(IVertex) = NFloor;
             LocMaxLevelVertexBot(IVertex) = NFloor;
      });

      OMEGA_SCOPE(LocVertexMask, VertexMask);
      parallelFor(
          {MyHorzMesh->NVerticesSize, NVertLevels}, KOKKOS_LAMBDA(int IVertex, int K) {
             LocVertexMask(IVertex, K) = (K < LocMaxLevelVertexTop(IVertex) ? 1. : 0.);
      });

      return Retval;
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
      I4 NVertLevels = DefHorzMesh->NVertLevels;
      I4 NChunks = NVertLevels / VecLength;

      auto *VertMesh = new VerticalMesh(DefHorzMesh);

      Real FloorFrac[8] = {0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875, 1.};
      for (int Iter = 0; Iter < 8; ++Iter) {
         I4 NFloor = NVertLevels * FloorFrac[Iter];
         RetVal += VertMesh->setFloor(NFloor);


         DivergenceWMask  DivWith(DefHorzMesh, VertMesh->CellMask);
//         DivergenceWMask2 DivWith2(DefHorzMesh, VertMesh->CellMask);
         DivergenceNoMask DivWout(DefHorzMesh);
         GradientWMask    GradWith(DefHorzMesh, VertMesh->EdgeMask);
//         GradientWMask2   GradWith2(DefHorzMesh, VertMesh->EdgeMask);
         GradientNoMask   GradWout(DefHorzMesh);
         CurlWMask        CurlWith(DefHorzMesh, VertMesh->VertexMask);
//         CurlWMask2       CurlWith2(DefHorzMesh, VertMesh->VertexMask);
         CurlNoMask       CurlWout(DefHorzMesh);

         Array2DReal CellOut("",DefHorzMesh->NCellsOwned, NVertLevels);
         Array2DReal EdgeIn("",DefHorzMesh->NEdgesSize, NVertLevels);

         Array2DReal EdgeOut("",DefHorzMesh->NEdgesOwned, NVertLevels);
         Array2DReal CellIn("",DefHorzMesh->NCellsSize, NVertLevels);

         Array2DReal VertOut("", DefHorzMesh->NVerticesOwned, NVertLevels);

         typedef Kokkos::TeamPolicy<>::member_type team_member;

         std::string CellMaskStr = "Cell Mask " + std::to_string(Iter);
         std::string CellMaskStr2 = "Cell Mask2 " + std::to_string(Iter);
         std::string CellHierarchStr = "Cell Hierarchical " + std::to_string(Iter);
         std::string CellMaskStr3 = "Tracer Cell Mask " + std::to_string(Iter);
         std::string CellHierarchStr3 = "Tracer Cell Hierarchical " + std::to_string(Iter);
         std::string EdgeMaskStr = "Edge Mask " + std::to_string(Iter);
         std::string EdgeMaskStr2 = "Edge Mask2 " + std::to_string(Iter);
         std::string EdgeHierarchStr = "Edge Hierarchical " + std::to_string(Iter);
         std::string EdgeMaskStr3 = "Tracer Edge Mask " + std::to_string(Iter);
         std::string EdgeHierarchStr3 = "Tracer Edge Hierarchical " + std::to_string(Iter);
         std::string VertexMaskStr = "Vertex Mask " + std::to_string(Iter);
         std::string VertexMaskStr2 = "Vertex Mask2 " + std::to_string(Iter);
         std::string VertexHierarchStr = "Vertex Hierarchical " + std::to_string(Iter);
         std::string VertexMaskStr3 = "Tracer Vertex Mask " + std::to_string(Iter);
         std::string VertexHierarchStr3 = "Tracer Vertex Hierarchical " + std::to_string(Iter);

         //for (int Rep = 0; Rep < 20; ++Rep) {
         for (int Rep = 0; Rep < 400; ++Rep) {

            std::cout << "Cell " << Iter << " " << Rep << std::endl;
            Kokkos::fence();
            Pacer::start(CellMaskStr);

            parallelFor(
                {DefHorzMesh->NCellsOwned, NChunks}, KOKKOS_LAMBDA(int ICell, int K) {
                   DivWith(CellOut, ICell, K, EdgeIn);
                });

            Kokkos::fence();
            Pacer::stop(CellMaskStr);

            OMEGA_SCOPE(LocCellMask, VertMesh->CellMask);

            Kokkos::fence();
            Pacer::start(CellMaskStr2);

            parallelFor(
                {DefHorzMesh->NCellsOwned, NChunks}, KOKKOS_LAMBDA(int ICell, int K) {
                   if (LocCellMask(ICell, K)) {
                      DivWith(CellOut, ICell, K, EdgeIn);
                   }
                });

            Kokkos::fence();
            Pacer::stop(CellMaskStr2);

 
            OMEGA_SCOPE(LocMaxLevelCell, VertMesh->MaxLevelCell);
            Kokkos::fence();
            Pacer::start(CellHierarchStr);
//            Kokkos::parallel_for("", Kokkos::TeamPolicy<>(DefHorzMesh->NCellsOwned, Kokkos::AUTO()),
            Kokkos::parallel_for("", Kokkos::TeamPolicy<>(DefHorzMesh->NCellsOwned, OMEGA_TEAMSIZE),
                                 KOKKOS_LAMBDA(const team_member& team) {
                   const int ICell = team.league_rank();
                   const int KMax = LocMaxLevelCell(ICell);
                   Kokkos::parallel_for(
                       Kokkos::TeamThreadRange(team, KMax),
                       [&](const int K) {
                          DivWout(CellOut, ICell, K, EdgeIn);
                       });
                });
            Kokkos::fence();
            Pacer::stop(CellHierarchStr);

//            Kokkos::fence();
//            Pacer::start(CellMaskStr3);
//
//            parallelFor(
//                {NNTracers, DefHorzMesh->NCellsOwned, NChunks}, KOKKOS_LAMBDA(int L, int ICell, int K) {
//                   DivWith(CellOut, ICell, K, EdgeIn);
//                });
//
//            Kokkos::fence();
//            Pacer::stop(CellMaskStr3);

//            Kokkos::fence();
//            Pacer::start(CellHierarchStr3);
//            Kokkos::parallel_for("", Kokkos::TeamPolicy<>(NNTracers * DefHorzMesh->NCellsOwned, OMEGA_TEAMSIZE),
//                                 KOKKOS_LAMBDA(const team_member& team) {
//                   const int L = team.league_rank() / DefHorzMesh->NCellsOwned;
//                   const int ICell = team.league_rank() % DefHorzMesh->NCellsOwned;
//                   const int KMax = LocMaxLevelCell(ICell);
//                   Kokkos::parallel_for(
//                       Kokkos::TeamThreadRange(team, KMax),
//                       [&](const int K) {
//                          DivWout(CellOut, ICell, K, EdgeIn);
//                       });
//                });
//            Kokkos::fence();
//            Pacer::stop(CellHierarchStr3);

            std::cout << "Edge " << Iter << " " << Rep << std::endl;

            Kokkos::fence();
            Pacer::start(EdgeMaskStr);

            parallelFor(
                {DefHorzMesh->NEdgesOwned, NChunks}, KOKKOS_LAMBDA(int IEdge, int K) {
                   GradWith(EdgeOut, IEdge, K, CellIn);
            });

            Kokkos::fence();
            Pacer::stop(EdgeMaskStr);

            OMEGA_SCOPE(LocEdgeMask, VertMesh->EdgeMask);

            Kokkos::fence();
            Pacer::start(EdgeMaskStr2);

            parallelFor(
                {DefHorzMesh->NEdgesOwned, NVertLevels}, KOKKOS_LAMBDA(int IEdge, int K) {
                   if (LocEdgeMask(IEdge, K)) {
                      GradWith(EdgeOut, IEdge, K, CellIn);
                   }
            });

            Kokkos::fence();
            Pacer::stop(EdgeMaskStr2);

            OMEGA_SCOPE(LocMaxLevelEdgeTop, VertMesh->MaxLevelEdgeTop);
            Kokkos::fence();
            Pacer::start(EdgeHierarchStr);
//            Kokkos::parallel_for("", Kokkos::TeamPolicy<>(DefHorzMesh->NEdgesOwned, Kokkos::AUTO()),
            Kokkos::parallel_for("", Kokkos::TeamPolicy<>(DefHorzMesh->NEdgesOwned, OMEGA_TEAMSIZE),
                                 KOKKOS_LAMBDA(const team_member& team) {
                   const int IEdge = team.league_rank();
                   const int KMax = LocMaxLevelEdgeTop(IEdge);
                   Kokkos::parallel_for(
                       Kokkos::TeamThreadRange(team, KMax),
                       [&](const int K) {
                          GradWout(EdgeOut, IEdge, K, CellIn);
                       });
                });
            Kokkos::fence();
            Pacer::stop(EdgeHierarchStr);

//            Kokkos::fence();
//            Pacer::start(EdgeMaskStr3);
//
//            parallelFor(
//                {NNTracers, DefHorzMesh->NEdgesOwned, NChunks}, KOKKOS_LAMBDA(int L, int IEdge, int K) {
//                   GradWith(EdgeOut, IEdge, K, CellIn);
//            });
//
//            Kokkos::fence();
//            Pacer::stop(EdgeMaskStr3);

//            Kokkos::fence();
//            Pacer::start(EdgeHierarchStr3);
//            Kokkos::parallel_for("", Kokkos::TeamPolicy<>(NNTracers * DefHorzMesh->NEdgesOwned, OMEGA_TEAMSIZE),
//                                 KOKKOS_LAMBDA(const team_member& team) {
//                   const int L = team.league_rank() / DefHorzMesh->NEdgesOwned;
//                   const int IEdge = team.league_rank() % DefHorzMesh->NEdgesOwned;
//                   const int KMax = LocMaxLevelEdgeTop(IEdge);
//                   Kokkos::parallel_for(
//                       Kokkos::TeamThreadRange(team, KMax),
//                       [&](const int K) {
//                          GradWout(EdgeOut, IEdge, K, CellIn);
//                       });
//                });
//            Kokkos::fence();
//            Pacer::stop(EdgeHierarchStr3);

            std::cout << "Vertex " << Iter << " " << Rep << std::endl;

            Kokkos::fence();
            Pacer::start(VertexMaskStr);

            parallelFor(
                {DefHorzMesh->NVerticesOwned, NChunks}, KOKKOS_LAMBDA(int IVertex, int K) {
                   CurlWith(VertOut, IVertex, K, EdgeIn);
            });

            Kokkos::fence();
            Pacer::stop(VertexMaskStr);

            OMEGA_SCOPE(LocVertexMask, VertMesh->VertexMask);

            Kokkos::fence();
            Pacer::start(VertexMaskStr2);

            parallelFor(
                {DefHorzMesh->NVerticesOwned, NVertLevels}, KOKKOS_LAMBDA(int IVertex, int K) {
                   if (LocVertexMask(IVertex, K)) {
                      CurlWith(VertOut, IVertex, K, EdgeIn);
                   }
            });

            Kokkos::fence();
            Pacer::stop(VertexMaskStr2);


            OMEGA_SCOPE(LocMaxLevelVertexTop, VertMesh->MaxLevelVertexTop);
            Kokkos::fence();
            Pacer::start(VertexHierarchStr);
            Kokkos::parallel_for("", Kokkos::TeamPolicy<>(DefHorzMesh->NVerticesOwned, OMEGA_TEAMSIZE),
                                 KOKKOS_LAMBDA(const team_member& team) {
                   const int IVertex = team.league_rank();
                   const int KMax = LocMaxLevelVertexTop(IVertex);
                   Kokkos::parallel_for(
                       Kokkos::TeamThreadRange(team, KMax),
                       [&](const int K) {
                          CurlWout(VertOut, IVertex, K, EdgeIn);
                       });
                });
            Kokkos::fence();
            Pacer::stop(VertexHierarchStr);

//            Kokkos::fence();
//            Pacer::start(VertexMaskStr3);
//
//            parallelFor(
//                {NNTracers, DefHorzMesh->NVerticesOwned, NChunks}, KOKKOS_LAMBDA(int L, int IVertex, int K) {
//                   CurlWith(VertOut, IVertex, K, EdgeIn);
//            });
//
//            Kokkos::fence();
//            Pacer::stop(VertexMaskStr3);

//            Kokkos::fence();
//            Pacer::start(VertexHierarchStr3);
//            Kokkos::parallel_for("", Kokkos::TeamPolicy<>(NNTracers, DefHorzMesh->NVerticesOwned, OMEGA_TEAMSIZE),
//                                 KOKKOS_LAMBDA(const team_member& team) {
//                   const int L = team.league_rank() / DefHorzMesh->NVerticesOwned;
//                   const int IVertex = team.league_rank() % DefHorzMesh->NVerticesOwned;
//                   const int KMax = LocMaxLevelVertexTop(IVertex);
//                   Kokkos::parallel_for(
//                       Kokkos::TeamThreadRange(team, KMax),
//                       [&](const int K) {
//                          CurlWout(VertOut, IVertex, K, EdgeIn);
//                       });
//                });
//            Kokkos::fence();
//            Pacer::stop(VertexHierarchStr3);

         }
      }

      HorzMesh::clear();
      Dimension::clear();
      Halo::clear();
      Decomp::clear();
      MachEnv::removeAll();
   }
   Pacer::print("omega");
   Pacer::finalize();

   Kokkos::finalize();
   MPI_Finalize();


   return RetVal;
} // end of main
