//===-- ocn/TendencyTerms.cpp - Tendency Terms ------------------*- C++ -*-===//
//
// The tendency terms that update state variables are implemented as functors,
// i.e. as classes that act like functions. This source defines the class
// constructors for these functors, which initialize the functor objects using
// the Mesh objects and info from the Config. The function call operators () are
// defined in the corresponding header file.
//
//===----------------------------------------------------------------------===//

#include "TendencyTerms.h"
#include "AuxiliaryState.h"
#include "DataTypes.h"
#include "HorzMesh.h"
#include "OceanState.h"
#include "Tracers.h"
#include "VertCoord.h"

namespace OMEGA {

ThicknessFluxDivOnCell::ThicknessFluxDivOnCell(const HorzMesh *Mesh,
                                               const VertCoord *VCoord)
    : MinLayerCell(VCoord->MinLayerCell), MaxLayerCell(VCoord->MaxLayerCell),
      MinLayerEdgeBot(VCoord->MinLayerEdgeBot),
      MaxLayerEdgeTop(VCoord->MaxLayerEdgeTop),
      NEdgesOnCell(Mesh->NEdgesOnCell), EdgesOnCell(Mesh->EdgesOnCell),
      DvEdge(Mesh->DvEdge), AreaCell(Mesh->AreaCell),
      EdgeSignOnCell(Mesh->EdgeSignOnCell) {}

PotentialVortHAdvOnEdge::PotentialVortHAdvOnEdge(const HorzMesh *Mesh,
                                                 const VertCoord *VCoord)
    : MinLayerEdgeBot(VCoord->MinLayerEdgeBot),
      MaxLayerEdgeTop(VCoord->MaxLayerEdgeTop),
      NEdgesOnEdge(Mesh->NEdgesOnEdge), EdgesOnEdge(Mesh->EdgesOnEdge),
      WeightsOnEdge(Mesh->WeightsOnEdge) {}

KEGradOnEdge::KEGradOnEdge(const HorzMesh *Mesh, const VertCoord *VCoord)
    : MinLayerEdgeBot(VCoord->MinLayerEdgeBot),
      MaxLayerEdgeTop(VCoord->MaxLayerEdgeTop), CellsOnEdge(Mesh->CellsOnEdge),
      DcEdge(Mesh->DcEdge) {}

SSHGradOnEdge::SSHGradOnEdge(const HorzMesh *Mesh, const VertCoord *VCoord)
    : MinLayerEdgeBot(VCoord->MinLayerEdgeBot),
      MaxLayerEdgeTop(VCoord->MaxLayerEdgeTop), CellsOnEdge(Mesh->CellsOnEdge),
      DcEdge(Mesh->DcEdge) {}

VelocityDiffusionOnEdge::VelocityDiffusionOnEdge(const HorzMesh *Mesh,
                                                 const VertCoord *VCoord)
    : MinLayerEdgeBot(VCoord->MinLayerEdgeBot),
      MaxLayerEdgeTop(VCoord->MaxLayerEdgeTop), CellsOnEdge(Mesh->CellsOnEdge),
      VerticesOnEdge(Mesh->VerticesOnEdge), DcEdge(Mesh->DcEdge),
      DvEdge(Mesh->DvEdge), MeshScalingDel2(Mesh->MeshScalingDel2) {}

VelocityHyperDiffOnEdge::VelocityHyperDiffOnEdge(const HorzMesh *Mesh,
                                                 const VertCoord *VCoord)
    : MinLayerEdgeBot(VCoord->MinLayerEdgeBot),
      MaxLayerEdgeTop(VCoord->MaxLayerEdgeTop), CellsOnEdge(Mesh->CellsOnEdge),
      VerticesOnEdge(Mesh->VerticesOnEdge), DcEdge(Mesh->DcEdge),
      DvEdge(Mesh->DvEdge), MeshScalingDel4(Mesh->MeshScalingDel4) {}

WindForcingOnEdge::WindForcingOnEdge(const HorzMesh *Mesh)
    : Enabled(false) {}

BottomDragOnEdge::BottomDragOnEdge(const HorzMesh *Mesh,
                                   const VertCoord *VCoord)
    : Enabled(false), Coeff(0), CellsOnEdge(Mesh->CellsOnEdge),
      MaxLayerEdgeTop(VCoord->MaxLayerEdgeTop),
      NVertLayers(VCoord->NVertLayers) {}

TracerHorzAdvOnCell::TracerHorzAdvOnCell(const HorzMesh *Mesh,
                                         const VertCoord *VCoord)
    : MinLayerCell(VCoord->MinLayerCell), MaxLayerCell(VCoord->MaxLayerCell),
      MinLayerEdgeBot(VCoord->MinLayerEdgeBot),
      MaxLayerEdgeTop(VCoord->MaxLayerEdgeTop),
      NEdgesOnCell(Mesh->NEdgesOnCell), EdgesOnCell(Mesh->EdgesOnCell),
      CellsOnEdge(Mesh->CellsOnEdge), EdgeSignOnCell(Mesh->EdgeSignOnCell),
      DvEdge(Mesh->DvEdge), AreaCell(Mesh->AreaCell) {
}

TracerDiffOnCell::TracerDiffOnCell(const HorzMesh *Mesh,
                                   const VertCoord *VCoord)
    : MinLayerCell(VCoord->MinLayerCell), MaxLayerCell(VCoord->MaxLayerCell),
      MinLayerEdgeBot(VCoord->MinLayerEdgeBot),
      MaxLayerEdgeTop(VCoord->MaxLayerEdgeTop),
      NEdgesOnCell(Mesh->NEdgesOnCell), EdgesOnCell(Mesh->EdgesOnCell),
      CellsOnEdge(Mesh->CellsOnEdge), EdgeSignOnCell(Mesh->EdgeSignOnCell),
      DvEdge(Mesh->DvEdge), DcEdge(Mesh->DcEdge), AreaCell(Mesh->AreaCell),
      MeshScalingDel2(Mesh->MeshScalingDel2) {}

TracerHyperDiffOnCell::TracerHyperDiffOnCell(const HorzMesh *Mesh,
                                             const VertCoord *VCoord)
    : MinLayerCell(VCoord->MinLayerCell), MaxLayerCell(VCoord->MaxLayerCell),
      MinLayerEdgeBot(VCoord->MinLayerEdgeBot),
      MaxLayerEdgeTop(VCoord->MaxLayerEdgeTop),
      NEdgesOnCell(Mesh->NEdgesOnCell), EdgesOnCell(Mesh->EdgesOnCell),
      CellsOnEdge(Mesh->CellsOnEdge), EdgeSignOnCell(Mesh->EdgeSignOnCell),
      DvEdge(Mesh->DvEdge), DcEdge(Mesh->DcEdge), AreaCell(Mesh->AreaCell),
      MeshScalingDel4(Mesh->MeshScalingDel4) {}

} // end namespace OMEGA

//===----------------------------------------------------------------------===//
