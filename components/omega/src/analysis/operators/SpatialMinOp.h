#ifndef OMEGA_GLOBALMINOP_H
#define OMEGA_GLOBALMINOP_H

//===----------------------------------------------------------------------===//
///
//===----------------------------------------------------------------------===//

#include "AnalysisOperator.h"
#include "Reductions.h"

namespace OMEGA {

template<typename ArrayT>
class SpatialMinOp : public AnalysisOperator {
 public:

   using ScalarT = typename ArrayT::non_const_value_type;

   SpatialMinOp(const std::string &Name, const Config &Options) {

      // Set operator type
      OperatorTypeName = "spatial_min";

      InstanceName = Name;
      InputNames = {InstanceName};

      std::string OutputFieldName = InstanceName + "_spatial_min";
      std::cout << OutputFieldName << std::endl;
      OutputNames = {OutputFieldName};

      // Initialize tracking variables
      FieldComputed = false;
      LastComputed = TimeInstant();

   }

   ~SpatialMinOp() override {
      if (Field::exists(OutputNames[0]))
         Field::destroy(OutputNames[0]);
   }

   void initialize(const Config *Options,
                   const MachEnv *InEnv,
                   const HorzMesh *MeshIn,
                   const VertCoord *VCoordIn) override {

      Mesh = MeshIn;
      VCoord = VCoordIn;
      Comm = InEnv->getComm();

      OutputData = typename Array1D<ScalarT>::type(OutputNames[0], 1);

      I4 NDims = 1;
      std::vector<std::string> DimNames(NDims);
      DimNames[0] = "Scalar";
      auto ScalarDim = Dimension::create(DimNames[0], 1);

      auto OutputField = Field::create(
         OutputNames[0],
         "Spatial minimum of " + InputNames[0], // Description
         "",                     // Units (inherited from input)
         "",                     // Standard name
         -std::numeric_limits<ScalarT>::max() / 10,// Min valid value
         std::numeric_limits<ScalarT>::max(), // Max valid value
         -std::numeric_limits<ScalarT>::max(), // Fill value
         NDims,                  // Dimension lengths
         DimNames                // Dimension names
      );

      OutputField->template attachData<typename Array1D<ScalarT>::type>(OutputData);

   }

   void compute(const TimeInstant &TimeStamp) override {

      auto InputField = Field::get(InputNames[0]);

      auto InputData = InputField->getDataArray<ArrayT>();

      std::vector<std::string> InputDimNames;

      InputField->getDimNames(InputDimNames);

      I4 NDims = InputDimNames.size();

      Array2DReal MaskArray;

      std::string IndexSpaceName = InputDimNames[std::max(0, NDims - 2)];

      if (IndexSpaceName == "NCells") {
         MaskArray = VCoord->CellMask;
      } else if (IndexSpaceName == "NEdges") {
         MaskArray = VCoord->EdgeMask;
      } else if (IndexSpaceName == "NVertices") {
         MaskArray = VCoord->VertexMask;
      } else {
         ABORT_ERROR("");
      }

//      dispatchFieldArray(*InputField, ComputeSpatialMin{Comm, MaskArray, SpatialMin});
      SpatialMin = globalWeightedMin(InputData, MaskArray, Comm);

      deepCopy(OutputData, SpatialMin);

   }

//   ScalarT getVal() {return SpatialMin;}

 private:

   // Member data
   const HorzMesh *Mesh;                    ///< Horizontal mesh
   const VertCoord *VCoord;                 ///< VertCoord
   MPI_Comm Comm;

   /// Output data storage - holds exactly one 1D array of data type
   /// matching input
   typename Array1D<ScalarT>::type OutputData;

   ScalarT SpatialMin;

};

} // namespace OMEGA
#endif
