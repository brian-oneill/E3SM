#ifndef OMEGA_ANALYSIS_H
#define OMEGA_ANALYSIS_H

//===-- analysis/Analysis.h - OMEGA Analysis --------------------*- C++ -*-===//
//
/// \file
/// \brief Defines core Analysis framework
///
///
//===----------------------------------------------------------------------===//

#include "AnalysisOperator.h"
#include "Config.h"
#include "DataTypes.h"
#include "Dimension.h"
#include "Error.h"
#include "Field.h"
#include "HorzMesh.h"
#include "Logging.h"
#include "TimeMgr.h"
#include "VertCoord.h"

#include <string>
#include <variant>

namespace OMEGA {

using Anlys1DVariant = std::variant<Array1DR4, Array1DR8, Array1DI4, Array1DI8>;
using Anlys2DVariant = std::variant<Array2DR4, Array2DR8, Array2DI4, Array2DI8>;
using Anlys3DVariant = std::variant<Array3DR4, Array3DR8, Array3DI4, Array3DI8>;

using Anlys1D2DVariant = std::variant<
   Array1DR4, Array1DR8, Array1DI4, Array1DI8,
   Array2DR4, Array2DR8, Array2DI4, Array2DI8>;
using Anlys2D3DVariant = std::variant<
   Array2DR4, Array2DR8, Array2DI4, Array2DI8,
   Array3DR4, Array3DR8, Array3DI4, Array3DI8>;

using AnlysAnyVariant = std::variant<
   Array1DR4, Array1DR8, Array1DI4, Array1DI8,  // 1D
   Array2DR4, Array2DR8, Array2DI4, Array2DI8,  // 2D
   Array3DR4, Array3DR8, Array3DI4, Array3DI8   // 3D
>;


} // namespace OMEGA
//===----------------------------------------------------------------------===//
#endif // OMEGA_ANALYSIS_H
