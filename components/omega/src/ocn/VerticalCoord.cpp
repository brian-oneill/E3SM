//===-- base/VerticalCoord.h - vertical coordinate --------------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//

#include "VerticalCoord.h"
#include <Kokkos_MinMax.hpp>

namespace OMEGA {

VertCoord *VertCoord::DefaultVertCoord = nullptr;
std::map<std::string, std::unique_ptr<VertCoord>> AllVertCoords;

//------------------------------------------------------------------------------
// Initialize default vertical coordinate, requires prior initialization of
// HorzMesh.

int VertCoord::init() {

   int Err = 0;

   HorzMesh *DefMesh = HorzMesh::getDefault();

   Config *OmegaConfig = Config::getOmegaConfig();

   VertCoord::DefaultVertCoord = create("Default", DefMesh, OmegaConfig);

   return Err;
} // end init

VertCoord::VertCoord(const HorzMesh *Mesh, Config *Options) {

} // end constructor

VertCoord *VertCoord::create(const std::string &Name, const HorzMesh *Mesh,
                             Config *Options) {
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
   auto *NewVertCoord = new VertCoord(Mesh, Options);
   AllVertCoords.emplace(Name, NewVertCoord);

   return NewVertCoord;
} // end create

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

   MinLevelEdgeTop = Array1DI4("MinLevelEdgeTop", NEdgesAll);
   MinLevelEdgeTop = Array1DI4("MinLevelEdgeTop", NEdgesAll);
   MaxLevelEdgeBot = Array1DI4("MaxLevelEdgeBot", NEdgesAll);
   MaxLevelEdgeBot = Array1DI4("MaxLevelEdgeBot", NEdgesAll);

   OMEGA_SCOPE(LocCellsOnEdge, CellsOnEdge);
   OMEGA_SCOPE(LocMinLevelCell, MinLevelCell);
   OMEGA_SCOPE(LocMaxLevelCell, MaxLevelCell);
   OMEGA_SCOPE(LocMinLevelEdgeTop, MinLevelEdgeTop);
   OMEGA_SCOPE(LocMinLevelEdgeBot, MinLevelEdgeBot);
   OMEGA_SCOPE(LocMaxLevelEdgeTop, MaxLevelEdgeTop);
   OMEGA_SCOPE(LocMaxLevelEdgeBot, MaxLevelEdgeBot);
   parallelFor(
       {NEdgesAll}, KOKKOS_LAMBDA(int IEdge) {
          const I4 ICell1 = LocCellsOnEdge(IEdge, 0);
          const I4 ICell2 = LocCellsOnEdge(IEdge, 1);
          LocMinLevelEdgeTop(IEdge) =
              Kokkos::min(LocMinLevelCell(ICell1), LocMinLevelCell(ICell2));
          LocMinLevelEdgeBot(IEdge) =
              Kokkos::max(LocMinLevelCell(ICell1), LocMinLevelCell(ICell2));

          LocMaxLevelEdgeTop(IEdge) =
              Kokkos::min(LocMaxLevelCell(ICell1), LocMaxLevelCell(ICell2));
          LocMaxLevelEdgeBot(IEdge) =
              Kokkos::max(LocMaxLevelCell(ICell1), LocMaxLevelCell(ICell2));
       });
}

//------------------------------------------------------------------------------
void VertCoord::minMaxLevelVertex() {

   MinLevelVertexTop = Array1DI4("MinLevelVertexTop", NVerticesAll);
   MinLevelVertexTop = Array1DI4("MinLevelVertexTop", NVerticesAll);
   MaxLevelVertexBot = Array1DI4("MaxLevelVertexBot", NVerticesAll);
   MaxLevelVertexBot = Array1DI4("MaxLevelVertexBot", NVerticesAll);

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
          I4 ICell                      = LocCellsOnVertex(IVertex, 0);
          LocMinLevelVertexBot(IVertex) = LocMinLevelCell(ICell);
          for (int I = 1; I < LocVertexDegree; ++I) {
             ICell                         = LocCellsOnVertex(IVertex, I);
             LocMinLevelVertexBot(IVertex) = Kokkos::max(
                 LocMinLevelVertexBot(IVertex), LocMinLevelCell(ICell));
          }

          ICell                         = LocCellsOnVertex(IVertex, 0);
          LocMinLevelVertexTop(IVertex) = LocMinLevelCell(ICell);
          for (int I = 1; I < LocVertexDegree; ++I) {
             ICell                         = LocCellsOnVertex(IVertex, I);
             LocMinLevelVertexTop(IVertex) = Kokkos::min(
                 LocMinLevelVertexTop(IVertex), LocMinLevelCell(ICell));
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
             LocMaxLevelVertexTop(IVertex) = Kokkos::min(
                 MaxLevelVertexTop(IVertex), LocMaxLevelCell(ICell));
          }
       });
}

//------------------------------------------------------------------------------
void VertCoord::computePressure(const Array2DReal &PressureInterface,
                                const Array2DReal &PressureMid,
                                const Array2DReal &LayerThickness,
                                const Array1DReal &SurfacePressure) {
   Real Gravity = 9.80616;
   Real Rho0    = 1.0;

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
       });
}

//------------------------------------------------------------------------------
void VertCoord::computeZHeight(const Array2DReal &ZInterface,
                               const Array2DReal &ZMid,
                               const Array2DReal &LayerThickness,
                               const Array2DReal &SpecVol,
                               const Array2DReal &BottomDepth) {}

//------------------------------------------------------------------------------
void VertCoord::computeGeopotential(const Array2DReal &GeopotentialMid,
                                    const Array2DReal &ZMid,
                                    const Array2DReal &TidalPotential,
                                    const Array2DReal &SelfAttractionLoading) {}

//------------------------------------------------------------------------------
void VertCoord::computePStarThickness(
    const Array2DReal &LayerThicknessPStar,
    const Array2DReal &VertCoordMovementWeights,
    const Array2DReal &RefLayerThickness) {}

} // end namespace OMEGA

//===----------------------------------------------------------------------===//
