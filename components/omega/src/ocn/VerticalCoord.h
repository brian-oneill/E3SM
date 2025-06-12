#ifndef OMEGA_VERTCOORD_H
#define OMEGA_VERTCOORD_H
//===-- base/VertCoord.h - vertical coordinate --------------*- C++ -*-===//
//
/// \file
/// \brief
///
///
//
//===----------------------------------------------------------------------===//

#include "Config.h"
#include "DataTypes.h"
#include "Error.h"
#include "HorzMesh.h"
#include "Logging.h"
#include "MachEnv.h"
#include "OmegaKokkos.h"

namespace OMEGA {

class VertCoord {

 private:
   // Variables from HorzMesh
   I4 NCellsOwned
   I4 NCellsAll;
   I4 NEdgesOwned;
   I4 NEdgesAll;
   I4 NVerticesOwned;
   I4 NVerticesAll;
   Array2DReal CellsOnEdge;
   Array2DReal CellsOnVertex;

   static VertCoord *DefaultVertCoord;
   static std::map<std::string, std::unique_ptr<VertCoord>> AllVertCoords;

   // methods

   /// construct a new vertical coordinate object
   VertCoord(const HorzMesh *Mesh,
             int NVertLevels,
             Config *Options
   );

   // Forbid copy and move construction
   VertCoord(const VertCoord &) = delete;
   VertCoord(VertCoord &&)      = delete;

   void minMaxLevelEdge(const Array1DReal &MinLevelCell,
                        const Array1DReal &MaxLevelCell);
   void minMaxLevelVertex(const Array1DReal &MinLevelCell,
                          const Array1DReal &MaxLevelCell);


 public:
   I4 NVertLevels;
   I4 NVertLevelsP1;

   // Variables computed 
   Array2DReal PressureInterface;
   Array2DReal PressureMid;
   Array2DReal ZInterface;
   Array2DReal ZMid;
   Array2DReal GeopotentialMid;
   Array2DReal LayerThicknessPStar;

   // Vertical loop bounds
   Array1DI4 MinLevelCell;
   Array1DI4 MaxLevelCell;
   Array1DI4 MinLevelEdgeTop;
   Array1DI4 MaxLevelEdgeTop;
   Array1DI4 MinLevelEdgeBot;
   Array1DI4 MaxLevelEdgeBot;
   Array1DI4 MinLevelVertexTop;
   Array1DI4 MaxLevelVertexTop;
   Array1DI4 MinLevelVertexBot;
   Array1DI4 MaxLevelVertexBot;

   // p star coordinate variables
   Array2DReal VertCoordMovementWeights;
   Array2DReal RefLayerThickness;

   // Variables from HorzMesh
   Array2DReal BottomDepth;

   // methods

   /// Initialize Omega vertical coordinate
   static int init();

   /// Creates a new vertical coordinate object by calling the constructor and
   /// puts it in the AllVertCoords map
   static VertCoord *create(const std::string &Name,
                            const HorzMesh *Mesh,
                            int NVertLevels,
                            Config *Options);

   /// Destructor - deallocates all memory and deletes a VertCoord
   ~VertCoord();

   static void clear();

   /// Remove a VertCoord by name
   static void erase(std::string InName
   );

   /// Retrieve the default VertCoord
   static VertCoord *getDefault();

   /// Retreive a VertCoord by name
   static VertCoord *get(std::string name);

   /// Sums the mass thickness times g from the top layer down, starting with
   /// the surface pressure
   void computePressure(const Array2DReal &PressureInterface,
                        const Array2DReal &PressureMid,
                        const Array2DReal &LayerThickness,
                        const Array1DReal &SurfacePressure
   );

   /// Sum the mass thickness time specific volume from the bottom layer up,
   /// starting with the bottom elevation
   void computeZHeight(const Array2DReal &ZInterface,
                       const Array2DReal &ZMid,
                       const Array2DReal &LayerThickness,
                       const Array2DReal &SpecVol,
                       const Array2DReal &BottomDepth
   );

   /// Sum the z height times g, the tidal potential, and self attraction and
   /// loading
   void computeGeopotential(const Array2DReal &GeopotentialMid,
                            const Array2DReal &ZMid,
                            const Array2DReal &TidalPotential,
                            const Array2DReal &SelfAttractionLoading
   );

   /// Determine mass thickness used for the p-star vertical coordinate
   void computePStarThickness(const Array2DReal &LayerThicknessPStar,
                              const Array2DReal &VertCoordMovementWeights,
                              const Array2DReal &RefLayerThickness
   );


}; // end class VertCoord

} // end namespace OMEGA

//===----------------------------------------------------------------------===//
#endif // defined OMEGA_VERTCOORD_H

