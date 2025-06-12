//===-- base/VerticalCoord.h - vertical coordinate --------------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//

#include "VerticalCoord.h"

namespace OMEGA {

VertCoord *VertCoord::DefaultVertCoord = nullptr;
std::map<std::string, std::unique_ptr<VertCoord>> AllVertCoords;

//------------------------------------------------------------------------------
// Initialize default vertical coordinate, requires prior initialization of
// HorzMesh.

int VertCoord::init() {

   int Err = 0;

   HorzMesh *DefMesh = HorzMesh::getDefault();

   I4 NVertLevels = DefMesh->NVertLevels;

   Config *OmegaConfig = Config::getOmegaConfig();

   VertCoord::DefaultVertCoord = create("Default", DefMesh, NVertLevels, OmegaConfig);

   return Err;
} // end init

VertCoord::VertCoord(const HorzMesh *Mesh,
                     int NVertLevels,
                     Config *Options
) {

} // end constructor

VertCoord *VertCoord::create(const std::string &Name,
                             const HorzMesh *Mesh,
                             int NVertLevels,
                             Config *Options
) {
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
   auto *NewVertCoord = new VertCoord(Mesh, NVertLevels, Options);
   AllVertCoords.emplace(Name, NewVertCoord);

   return NewVertCoord;
} // end create

//------------------------------------------------------------------------------
// Destroys a local VertCoord and deallocates all arrays
VertCoord::~VertCoord() {

} // end deconstructor

//------------------------------------------------------------------------------
// Removes a VertCoord from list by name
void VertCoord::erase(std::string InName
) {
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
void VertCoord::minMaxLevelEdge(const Array1DReal &MinLevelCell,
                                const Array1DReal &MaxLevelCell) {



}

//------------------------------------------------------------------------------
void VertCoord::minMaxLevelVertex(const Array1DReal &MinLevelCell,
                                  const Array1DReal &MaxLevelCell) {

}

//------------------------------------------------------------------------------
void VertCoord::computePressure(const Array2DReal &PressureInterface,
                                const Array2DReal &PressureMid,
                                const Array2DReal &LayerThickness,
                                const Array1DReal &SurfacePressure) {
   Real Gravity = 9.80616;
   Real Rho0 = 1.0;

   OMEGA_SCOPE(LocMinLevelCell, MinLevelCell);
   OMEGA_SCOPE(LocMaxLevelCell, MaxLevelCell);

   const auto Policy = TeamPolicy(NCellsAll, OMEGA_TEAMSIZE, 1);
   Kokkos::parallel_for("computePressure", Policy,
                        KOKKOS_LAMBDA(const TeamMember &Member) {
      const I4 ICell = Member.league_rank();
      const I4 KMin = LocMinLevelCell(ICell);
      const I4 KMax = LocMaxLevelCell(ICell);
      PressureInterface(ICell, KMin) = SurfacePressure(ICell);
      PressureMid(ICell, KMin) = SurfacePressure(ICell) + 0.5_Real * Gravity *
                                 Rho0 * LayerThickness(ICell, Kmin);

   });

}

//------------------------------------------------------------------------------
void VertCoord::computeZHeight(const Array2DReal &ZInterface,
                               const Array2DReal &ZMid,
                               const Array2DReal &LayerThickness,
                               const Array2DReal &SpecVol,
                               const Array2DReal &BottomDepth)
{

}

//------------------------------------------------------------------------------
void VertCoord::computeGeopotential(const Array2DReal &GeopotentialMid,
                                    const Array2DReal &ZMid,
                                    const Array2DReal &TidalPotential,
                                    const Array2DReal &SelfAttractionLoading)
{

}

//------------------------------------------------------------------------------
void VertCoord::computePStarThickness(const Array2DReal &LayerThicknessPStar,
                                       const Array2DReal &VertCoordMovementWeights,
                                       const Array2DReal &RefLayerThickness)
{

}

} // end namespace OMEGA

//===----------------------------------------------------------------------===//
