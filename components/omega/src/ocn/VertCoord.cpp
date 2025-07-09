//===-- base/VertCoord.cpp - vertical coordinate ----------------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//

#include "VertCoord.h"
#include "Dimension.h"
#include "IO.h"
#include "Logging.h"
#include "MachEnv.h"
#include "OmegaKokkos.h"

#include <iostream>

namespace OMEGA {

VertCoord *VertCoord::DefaultVertCoord = nullptr;
std::map<std::string, std::unique_ptr<VertCoord>> VertCoord::AllVertCoords;

//------------------------------------------------------------------------------
// Initialize default vertical coordinate, requires prior initialization of
// HorzMesh.
void VertCoord::init() {

   int Err = 0;

   HorzMesh *DefMesh = HorzMesh::getDefault();

   Decomp *DefDecomp = Decomp::getDefault();

   Config *OmegaConfig = Config::getOmegaConfig();

   VertCoord::DefaultVertCoord =
       create("Default", DefMesh, DefDecomp, OmegaConfig);

} // end init

VertCoord::VertCoord(const HorzMesh *Mesh, const Decomp *MeshDecomp,
                     Config *Options) {

   // Retrieve mesh filename from HorzMesh
   MeshFileName = Mesh->MeshFileName;

   // Set NVertLevels and NVertLevelsP1
   NVertLevels   = Mesh->NVertLevels;
   NVertLevelsP1 = NVertLevels + 1;

   // Retrieve mesh variables from HorzMesh
   NCellsOwned = Mesh->NCellsOwned;
   NCellsAll   = Mesh->NCellsAll;
   NCellsSize  = Mesh->NCellsSize;

   NEdgesOwned = Mesh->NEdgesOwned;
   NEdgesAll   = Mesh->NEdgesAll;
   NEdgesSize  = Mesh->NEdgesSize;

   NVerticesOwned = Mesh->NVerticesOwned;
   NVerticesAll   = Mesh->NVerticesAll;
   NVerticesSize  = Mesh->NVerticesSize;
   VertexDegree   = Mesh->VertexDegree;

   // Retrieve connectivity arrays from HorzMesh
   CellsOnEdge   = Mesh->CellsOnEdge;
   CellsOnVertex = Mesh->CellsOnVertex;

   // Allocate device arrays
   PressureInterface =
       Array2DReal("PressureInterface", NCellsSize, NVertLevelsP1);
   PressureMid = Array2DReal("PressureMid", NCellsSize, NVertLevels);

   ZInterface = Array2DReal("ZInterface", NCellsSize, NVertLevelsP1);
   ZMid       = Array2DReal("ZMid", NCellsSize, NVertLevels);

   GeopotentialMid = Array2DReal("GeopotentialMid", NCellsSize, NVertLevels);
   LayerThicknessPStar =
       Array2DReal("LayerThicknessPStar", NCellsSize, NVertLevels);

   PressureInterfaceH = createHostMirrorCopy(PressureInterface);
   PressureMidH       = createHostMirrorCopy(PressureMid);
   ZInterfaceH        = createHostMirrorCopy(ZInterface);
   ZMidH              = createHostMirrorCopy(ZMid);

   // Open the mesh file for reading (assume IO has already been initialized)
   I4 Err;
   Err = OMEGA::IO::openFile(MeshFileID, MeshFileName, IO::ModeRead);

   readMinMaxCell(MeshDecomp);

   minMaxLevelEdge();
   minMaxLevelVertex();

} // end constructor

VertCoord *VertCoord::create(const std::string &Name, const HorzMesh *Mesh,
                             const Decomp *MeshDecomp, Config *Options) {
   // Check to see if a VertCoord of the same name already exists and, if so,
   // exit with an error
   if (AllVertCoords.find(Name) != AllVertCoords.end()) {
      LOG_ERROR("Attempted to create a VertCoord with name {} but a VertCoord "
                "of that name already exists",
                Name);
      return nullptr;
   }

   // create a new VertCoord on the heap and put it in a map of unique_ptrs,
   // which will manage its lifetime
   auto *NewVertCoord = new VertCoord(Mesh, MeshDecomp, Options);
   AllVertCoords.emplace(Name, NewVertCoord);

   return NewVertCoord;
} // end create

void VertCoord::readMinMaxCell(const Decomp *MeshDecomp) {

   int Err = 0;
   //   // Create offset and Dimension for NCells
   //   std::string DimName = "NCells";
   //   HostArray1DI4 CellOffset("CellOffset", NCellsSize);
   //   I4 NCellsGlobal = MeshDecomp->NCellsGlobal;
   //   for (int Cell = 0; Cell < NCellsSize; ++Cell) {
   //      if (Cell < NCellsOwned) {
   //         CellOffset(Cell) = MeshDecomp->CellIDH(Cell) - 1;
   //      } else {
   //         CellOffset(Cell) = -1;
   //      }
   //   }
   //
   //   auto NCellsDim = Dimension::create(DimName, NCellsGlobal, NCellsSize,
   //   CellOffset);

   I4 NDims             = 1;
   IO::Rearranger Rearr = IO::RearrBox;

   I4 CellDecompI4;
   std::vector<I4> CellDims{MeshDecomp->NCellsGlobal};
   std::vector<I4> CellID(NCellsAll);
   for (int Cell = 0; Cell < NCellsAll; ++Cell) {
      CellID[Cell] = MeshDecomp->CellIDH(Cell) - 1;
   }
   I4 DecErr = IO::createDecomp(CellDecompI4, IO::IOTypeI4, NDims, CellDims,
                                NCellsAll, CellID, Rearr);
   if (DecErr != 0) {
      LOG_CRITICAL("VertCoord: error creating cell IO decomposition");
      ++Err;
   }

   HostArray1DI4 TmpArrayH("TmpCellArray", NCellsSize);
   I4 ArrayID;
   const std::string MaxNameMPAS = "maxLevelCell";
   I4 MaxErr = IO::readArray(TmpArrayH.data(), NCellsAll, MaxNameMPAS,
                             MeshFileID, CellDecompI4, ArrayID);

   //   std::cout << " max err : " << MaxErr << std::endl;
   //   for (int I = 0; I < NCellsAll; ++I) {
   //      std::cout << "   " << I << " " << TmpArrayH(I) << std::endl;
   //
   //   }

   if (MaxErr != 0) {
      LOG_WARN("VertCoord: error reading maxLevelCell from mesh file, ",
               "using Max = NVertLevels - 1");
      deepCopy(TmpArrayH, NVertLevels - 1);
   } else {
      for (int ICell = 0; ICell < NCellsAll; ++ICell) {
         TmpArrayH(ICell) = TmpArrayH(ICell) - 1;
      }
   }
   TmpArrayH(NCellsAll) = -1;

   MaxLevelCellH = HostArray1DI4("MaxLevelCell", NCellsSize);
   deepCopy(MaxLevelCellH, TmpArrayH);
   MaxLevelCell = createDeviceMirrorCopy(MaxLevelCellH);

   const std::string MinNameMPAS = "minLevelCell";
   I4 MinErr = IO::readArray(TmpArrayH.data(), NCellsAll, MinNameMPAS,
                             MeshFileID, CellDecompI4, ArrayID);

   //   std::cout << " min err : " << MinErr << std::endl;

   if (MinErr != 0) {
      LOG_WARN("VertCoord: error reading minLevelCell from mesh file, ",
               "using Min = 0");
      deepCopy(TmpArrayH, 0);
   } else {
      for (int ICell = 0; ICell < NCellsAll; ++ICell) {
         TmpArrayH(ICell) = TmpArrayH(ICell) - 1;
      }
   }
   TmpArrayH(NCellsAll) = NVertLevelsP1;

   MinLevelCellH = HostArray1DI4("MinLevelCell", NCellsSize);
   deepCopy(MinLevelCellH, TmpArrayH);
   MinLevelCell = createDeviceMirrorCopy(MinLevelCellH);

   //   for (int I = 0; I < NCellsAll; ++I) {
   //      std::cout << "   " << I << " " << MinLevelCellH(I) << " " <<
   //      MaxLevelCellH(I) << std::endl;
   //
   //   }

   Err = IO::destroyDecomp(CellDecompI4);
}

//------------------------------------------------------------------------------
// Destroys a local VertCoord and deallocates all arrays
VertCoord::~VertCoord() {} // end deconstructor

//------------------------------------------------------------------------------
// Removes a VertCoord from list by name
void VertCoord::erase(std::string InName) {
   AllVertCoords.erase(InName); // removes the VertCoord from the list and in
                                // the process, calls the destructor
} // end erase

//------------------------------------------------------------------------------
// Removes all VertCoords to clean up before exit
void VertCoord::clear() {

   AllVertCoords.clear(); // removes all VertCoords from the list and in the
                          // process, calls the destructors for each

} // end clear

//------------------------------------------------------------------------------
void VertCoord::minMaxLevelEdge() {

   MinLevelEdgeTop = Array1DI4("MinLevelEdgeTop", NEdgesSize);
   MinLevelEdgeBot = Array1DI4("MinLevelEdgeBot", NEdgesSize);
   MaxLevelEdgeTop = Array1DI4("MaxLevelEdgeTop", NEdgesSize);
   MaxLevelEdgeBot = Array1DI4("MaxLevelEdgeBot", NEdgesSize);

   OMEGA_SCOPE(LocNVertLevelsP1, NVertLevelsP1);
   OMEGA_SCOPE(LocCellsOnEdge, CellsOnEdge);
   OMEGA_SCOPE(LocMinLevelCell, MinLevelCell);
   OMEGA_SCOPE(LocMaxLevelCell, MaxLevelCell);
   OMEGA_SCOPE(LocMinLevelEdgeTop, MinLevelEdgeTop);
   OMEGA_SCOPE(LocMinLevelEdgeBot, MinLevelEdgeBot);
   OMEGA_SCOPE(LocMaxLevelEdgeTop, MaxLevelEdgeTop);
   OMEGA_SCOPE(LocMaxLevelEdgeBot, MaxLevelEdgeBot);
   parallelFor(
       {NEdgesAll}, KOKKOS_LAMBDA(int IEdge) {
          I4 Lvl1;
          I4 Lvl2;
          const I4 ICell1 = LocCellsOnEdge(IEdge, 0);
          const I4 ICell2 = LocCellsOnEdge(IEdge, 1);
          Lvl1            = LocMaxLevelCell(ICell1) == -1 ? LocNVertLevelsP1
                                                          : LocMinLevelCell(ICell1);
          Lvl2            = LocMaxLevelCell(ICell2) == -1 ? LocNVertLevelsP1
                                                          : LocMinLevelCell(ICell2);
          LocMinLevelEdgeTop(IEdge) = Kokkos::min(Lvl1, Lvl2);

          Lvl1 = LocMaxLevelCell(ICell1) == -1 ? -1 : LocMinLevelCell(ICell1);
          Lvl2 = LocMaxLevelCell(ICell2) == -1 ? -1 : LocMinLevelCell(ICell2);
          LocMinLevelEdgeBot(IEdge) = Kokkos::max(Lvl1, Lvl2);

          LocMaxLevelEdgeTop(IEdge) =
              Kokkos::min(LocMaxLevelCell(ICell1), LocMaxLevelCell(ICell2));
          LocMaxLevelEdgeBot(IEdge) =
              Kokkos::max(LocMaxLevelCell(ICell1), LocMaxLevelCell(ICell2));

          //          std::cout << IEdge << "   " << DefDecomp->CellIDH(ICell1)
          //          << "   "; std::cout << DefDecomp->CellIDH(ICell2) << " ";
          //          std::cout << LocMaxLevelEdgeTop(IEdge) << "   ";
          //          std::cout << LocMaxLevelEdgeBot(IEdge) << "   ";
          //          std::cout << LocMinLevelEdgeTop(IEdge) << "   ";
          //          std::cout << LocMinLevelEdgeBot(IEdge) << std::endl;
       });
   OMEGA_SCOPE(LocNEdgesAll, NEdgesAll);
   parallelFor(
       {1}, KOKKOS_LAMBDA(const int &) {
          LocMinLevelEdgeTop(LocNEdgesAll) = LocNVertLevelsP1;
          LocMinLevelEdgeBot(LocNEdgesAll) = LocNVertLevelsP1;
          LocMaxLevelEdgeTop(LocNEdgesAll) = -1;
          LocMaxLevelEdgeBot(LocNEdgesAll) = -1;
       });

   MinLevelEdgeTopH = createHostMirrorCopy(MinLevelEdgeTop);
   MinLevelEdgeBotH = createHostMirrorCopy(MinLevelEdgeBot);
   MaxLevelEdgeTopH = createHostMirrorCopy(MaxLevelEdgeTop);
   MaxLevelEdgeBotH = createHostMirrorCopy(MaxLevelEdgeBot);
}

//------------------------------------------------------------------------------
void VertCoord::minMaxLevelVertex() {

   MinLevelVertexTop = Array1DI4("MinLevelVertexTop", NVerticesSize);
   MinLevelVertexBot = Array1DI4("MinLevelVertexBot", NVerticesSize);
   MaxLevelVertexTop = Array1DI4("MaxLevelVertexTop", NVerticesSize);
   MaxLevelVertexBot = Array1DI4("MaxLevelVertexBot", NVerticesSize);

   OMEGA_SCOPE(LocNVertLevelsP1, NVertLevelsP1);
   OMEGA_SCOPE(LocVertexDegree, VertexDegree);
   OMEGA_SCOPE(LocCellsOnVertex, CellsOnVertex);
   OMEGA_SCOPE(LocMinLevelCell, MinLevelCell);
   OMEGA_SCOPE(LocMaxLevelCell, MaxLevelCell);
   OMEGA_SCOPE(LocMinLevelVertexTop, MinLevelVertexTop);
   OMEGA_SCOPE(LocMinLevelVertexBot, MinLevelVertexBot);
   OMEGA_SCOPE(LocMaxLevelVertexTop, MaxLevelVertexTop);
   OMEGA_SCOPE(LocMaxLevelVertexBot, MaxLevelVertexBot);
   parallelFor(
       {NVerticesAll}, KOKKOS_LAMBDA(int IVertex) {
          I4 Lvl;
          I4 ICell = LocCellsOnVertex(IVertex, 0);
          Lvl      = LocMaxLevelCell(ICell) == -1 ? -1 : LocMinLevelCell(ICell);
          LocMinLevelVertexBot(IVertex) = Lvl;
          for (int I = 1; I < LocVertexDegree; ++I) {
             ICell = LocCellsOnVertex(IVertex, I);
             Lvl   = LocMaxLevelCell(ICell) == -1 ? -1 : LocMinLevelCell(ICell);
             LocMinLevelVertexBot(IVertex) =
                 Kokkos::max(LocMinLevelVertexBot(IVertex), Lvl);
          }

          ICell = LocCellsOnVertex(IVertex, 0);
          Lvl   = LocMaxLevelCell(ICell) == -1 ? LocNVertLevelsP1
                                               : LocMinLevelCell(ICell);
          LocMinLevelVertexTop(IVertex) = Lvl;
          for (int I = 1; I < LocVertexDegree; ++I) {
             ICell = LocCellsOnVertex(IVertex, I);
             Lvl   = LocMaxLevelCell(ICell) == -1 ? LocNVertLevelsP1
                                                  : LocMinLevelCell(ICell);
             LocMinLevelVertexTop(IVertex) =
                 Kokkos::min(LocMinLevelVertexTop(IVertex), Lvl);
          }

          ICell                         = LocCellsOnVertex(IVertex, 0);
          LocMaxLevelVertexBot(IVertex) = LocMaxLevelCell(ICell);
          for (int I = 1; I < LocVertexDegree; ++I) {
             ICell                         = LocCellsOnVertex(IVertex, I);
             LocMaxLevelVertexBot(IVertex) = Kokkos::max(
                 LocMaxLevelVertexBot(IVertex), LocMaxLevelCell(ICell));
          }

          ICell                         = LocCellsOnVertex(IVertex, 0);
          LocMaxLevelVertexTop(IVertex) = LocMaxLevelCell(ICell);
          for (int I = 1; I < LocVertexDegree; ++I) {
             ICell                         = LocCellsOnVertex(IVertex, I);
             LocMaxLevelVertexTop(IVertex) = Kokkos::min(
                 MaxLevelVertexTop(IVertex), LocMaxLevelCell(ICell));
          }
       });

   OMEGA_SCOPE(LocNVerticesAll, NVerticesAll);
   parallelFor(
       {1}, KOKKOS_LAMBDA(const int &) {
          LocMinLevelVertexTop(LocNVerticesAll) = LocNVertLevelsP1;
          LocMinLevelVertexBot(LocNVerticesAll) = LocNVertLevelsP1;
          LocMaxLevelVertexTop(LocNVerticesAll) = -1;
          LocMaxLevelVertexBot(LocNVerticesAll) = -1;
       });

   MinLevelVertexTopH = createHostMirrorCopy(MinLevelVertexTop);
   MinLevelVertexBotH = createHostMirrorCopy(MinLevelVertexBot);
   MaxLevelVertexTopH = createHostMirrorCopy(MaxLevelVertexTop);
   MaxLevelVertexBotH = createHostMirrorCopy(MaxLevelVertexBot);
}

//------------------------------------------------------------------------------
void VertCoord::computePressure(const Array2DReal &PressureInterface,
                                const Array2DReal &PressureMid,
                                const Array2DReal &LayerThickness,
                                const Array1DReal &SurfacePressure) {

   Real Gravity = 9.80616_Real;
   Real Rho0    = 1035._Real;

   OMEGA_SCOPE(LocMinLevelCell, MinLevelCell);
   OMEGA_SCOPE(LocMaxLevelCell, MaxLevelCell);

   const auto Policy = TeamPolicy(NCellsAll, OMEGA_TEAMSIZE, 1);
   Kokkos::parallel_for(
       "computePressure", Policy, KOKKOS_LAMBDA(const TeamMember &Member) {
          const I4 ICell                 = Member.league_rank();
          const I4 KMin                  = LocMinLevelCell(ICell);
          const I4 KMax                  = LocMaxLevelCell(ICell);
          const I4 Range                 = KMax - KMin - 1;
          PressureInterface(ICell, KMin) = SurfacePressure(ICell);
          PressureMid(ICell, KMin) =
              SurfacePressure(ICell) +
              0.5_Real * Gravity * Rho0 * LayerThickness(ICell, KMin);
          Kokkos::parallel_scan(
              TeamThreadRange(Member, Range),
              [&](int K, Real &Accum, bool IsFinal) {
                 const I4 KLvl = K + KMin;
                 Accum += PressureInterface(ICell, KLvl) +
                          Gravity * Rho0 * LayerThickness(ICell, KLvl);
                 if (IsFinal) {
                    PressureInterface(ICell, KLvl + 1) = Accum;
                    PressureMid(ICell, KLvl + 1) =
                        Accum +
                        0.5_Real * Gravity * Rho0 * LayerThickness(ICell, KLvl);
                 }
              });
          Member.team_barrier();
          PressureInterface(ICell, KMax + 1) =
              PressureInterface(ICell, KMax) +
              Gravity * Rho0 * LayerThickness(ICell, KMax);
       });
}

//------------------------------------------------------------------------------
void VertCoord::computeZHeight(const Array2DReal &ZInterface,
                               const Array2DReal &ZMid,
                               const Array2DReal &LayerThickness,
                               const Array2DReal &SpecVol,
                               const Array1DReal &BottomDepth) {

   Real Rho0 = 1035._Real;

   OMEGA_SCOPE(LocMinLevelCell, MinLevelCell);
   OMEGA_SCOPE(LocMaxLevelCell, MaxLevelCell);

   const auto Policy = TeamPolicy(NCellsAll, OMEGA_TEAMSIZE, 1);
   Kokkos::parallel_for(
       "computePressure", Policy, KOKKOS_LAMBDA(const TeamMember &Member) {
          const I4 ICell = Member.league_rank();
          const I4 KMin  = LocMinLevelCell(ICell);
          const I4 KMax  = LocMaxLevelCell(ICell);
          const I4 Range = KMax - KMin - 1;

          ZInterface(ICell, KMax + 1) = -BottomDepth(ICell);
          ZMid(ICell, KMax) =
              -BottomDepth(ICell) + 0.5_Real * Rho0 * SpecVol(ICell, KMax) *
                                        LayerThickness(ICell, KMax);
          Kokkos::parallel_scan(
              TeamThreadRange(Member, Range),
              [&](int K, Real &Accum, bool IsFinal) {
                 const I4 KLvl = KMax - K;
                 Accum +=
                     ZInterface(ICell, KLvl + 1) +
                     Rho0 * SpecVol(ICell, KLvl) * LayerThickness(ICell, KLvl);
                 if (IsFinal) {
                    ZInterface(ICell, KLvl) = Accum;
                    ZMid(ICell, KLvl - 1) =
                        Accum + 0.5_Real * Rho0 * SpecVol(ICell, KLvl - 1) *
                                    LayerThickness(ICell, KLvl - 1);
                 }
              });
          Member.team_barrier();
          ZInterface(ICell, KMin) =
              ZInterface(ICell, KMin + 1) +
              Rho0 * SpecVol(ICell, KMin) * LayerThickness(ICell, KMin)
       });
}

//------------------------------------------------------------------------------
void VertCoord::computeGeopotential(const Array2DReal &GeopotentialMid,
                                    const Array2DReal &ZMid,
                                    const Array2DReal &TidalPotential,
                                    const Array2DReal &SelfAttractionLoading) {

   Real Gravity = 9.80616_Real;

   OMEGA_SCOPE(LocMinLevelCell, MinLevelCell);
   OMEGA_SCOPE(LocMaxLevelCell, MaxLevelCell);

   Kokkos::parallel(
       "computeGeopotential", TeamPolicy(NCellsAll, OMEGA_TEAMSIZE),
       KOKKOS_LAMBDA(const TeamMember &Team) {
          const I4 ICell   = Member.league_rank();
          const I4 KMin    = LocMinLevelCell(ICell);
          const I4 KMax    = LocMaxLevelCell(ICell);
          const I4 KRange  = KMax - KMin + 1;
          const I4 NChunks = (KRange + VecLength - 1) / VecLength;
          Kokkos::parallel_for(
              Kokkos::TeamThreadRange(Team, NChunks), [&](const int KChunk) {
                 const I4 KStart = KMin + KChunk * VecLength;
                 const I4 KEnd   = KStart + VecLength;

                 const I4 KLen =
                     KEnd > KMax + 1 ? KMax + 1 - KStart : VecLength;
                 for (int KVec = 0; KVec < KLen; ++KVec) {
                    const I4 K                = KStart + KVec;
                    GeopotentialMid(ICell, K) = Gravity * ZMid(ICell, K) +
                                                TidalPotential(ICell, K) +
                                                SelfAttractionLoading(ICell, K);
                 }
              });
       });
}

//------------------------------------------------------------------------------
void VertCoord::computePStarThickness(
    const Array2DReal &LayerThicknessPStar,
    const Array2DReal &VertCoordMovementWeights,
    const Array2DReal &RefLayerThickness) {

   Real Gravity = 9.80616_Real;
   Real Rho0    = 1035._Real;

   OMEGA_SCOPE(LocMinLevelCell, MinLevelCell);
   OMEGA_SCOPE(LocMaxLevelCell, MaxLevelCell);
   OMEGA_SCOPE(LocPressInterf, PressureInterface);

   Kokkos::parallel(
       "computeGeopotential", TeamPolicy(NCellsAll, OMEGA_TEAMSIZE),
       KOKKOS_LAMBDA(const TeamMember &Team) {
          const I4 ICell = Member.league_rank();
          const I4 KMin  = LocMinLevelCell(ICell);
          const I4 KMax  = LocMaxLevelCell(ICell);

          Real Coeff = (LocPressInterf(KMax + 1) - LocPressInterf(KMin)) /
                       (Gravity * Rho0);

          Real SumWh = 0;
          Kokkos::parallel_reduce(
              Kokkos::TeamThreadRange(Team, KMin, KMax),
              [=](const int K, Real &LocalWh) {
                 LocalWh += VertCoordMovementWeights(ICell, K) *
                            RefLayerThickness(ICell, K);
              },
              SumWh);

          const I4 KRange  = KMax - KMin + 1;
          const I4 NChunks = (KRange + VecLength - 1) / VecLength;
          Member.team_barrier();

          Kokkos::parallel_for(
              Kokkos::TeamThreadRange(Team, NChunks), [&](const int KChunk) {
                 const I4 KStart = KMin + KChunk * VecLength;
                 const I4 KEnd   = KStart + VecLength;

                 const I4 KLen =
                     KEnd > KMax + 1 ? KMax + 1 - KStart : VecLength;
                 for (int KVec = 0; KVec < KLen; ++KVec) {
                    const I4 K = KStart + KVec;
                    LayerThicknessPStar(ICell, K) =
                        RefLayerThickness(ICell, K) *
                        (1._Real +
                         Coeff * VertCoordMovementWeights(ICell, K) / SumWh);
                 }
              });
       });
}

//------------------------------------------------------------------------------
// Get default VertCoord
VertCoord *VertCoord::getDefault() { return VertCoord::DefaultVertCoord; }

//------------------------------------------------------------------------------
// Get VertCoord by name
VertCoord *VertCoord::get(const std::string Name ///< [in] Name of VertCoord
) {

   // look for an instance of this name
   auto it = AllVertCoords.find(Name);

   // if found, return the VertCoord pointer
   if (it != AllVertCoords.end()) {
      return it->second.get();

      // otherwise print error and return null pointer
   } else {
      LOG_ERROR("VertCoord::get: Attempt to retrieve non-existant VertCoord:");
      LOG_ERROR("{} has not been defined or has been removed", Name);
      return nullptr;
   }

} // end get VertCoord

} // end namespace OMEGA

//===----------------------------------------------------------------------===//
