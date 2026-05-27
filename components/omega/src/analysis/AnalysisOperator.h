#ifndef OMEGA_ANALYSISOP_H
#define OMEGA_ANALYSISOP_H

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



class AnalysisOperator {


 public:
   virtual ~AnalysisOperator() = default;

   /// Return name for this operator type
   const std::string getOperatorType();

   /// Return unique name for this instance of the operator type, contains
   /// concatenated strings of upstream operator Names
   const std::string getName();

   /// Return names of fields required by this operator
   const std::vector<std::string> getInputFieldNames();

   /// Return names of output fields produced by this operator
   const std::vector<std::string> getOutputFieldNames();

   /// Initialize operator: create and register output fields in Field map
   virtual void initialize(const Config *Options,
                           const HorzMesh *Mesh,
                           const VertCoord *VCoord) = 0;

   /// Perform computation of Analysis fields. Data arrays of input field
   /// retrieved from Field map using input field names. Writes to
   /// operator-owned output arrays
   virtual void compute(const TimeInstant& ts) = 0;

 protected:
   std::string OperatorTypeName;
   std::string InstanceName;
   std::vector<std::string> InputNames;
   std::vector<std::string> OutputNames;

 
   TimeInstant LastComputed;
   bool FieldComputed;
};

}

#endif
