#ifndef OMEGA_ANALYSISOP_H
#define OMEGA_ANALYSISOP_H

#include "Config.h"
#include "DataTypes.h"
#include "Dimension.h"
#include "Error.h"
#include "Field.h"
#include "HorzMesh.h"
#include "Logging.h"
#include "MachEnv.h"
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
   Array1DR4, Array1DR8, Array1DI4, Array1DI8, 
   Array2DR4, Array2DR8, Array2DI4, Array2DI8, 
   Array3DR4, Array3DR8, Array3DI4, Array3DI8  
>;

template<typename T> struct Array1D;
template<> struct Array1D<I4> { using type = Array1DI4; };
template<> struct Array1D<I8> { using type = Array1DI8; };
template<> struct Array1D<R4> { using type = Array1DR4; };
template<> struct Array1D<R8> { using type = Array1DR8; };

template<typename T> struct Array2D;
template<> struct Array2D<I4> { using type = Array2DI4; };
template<> struct Array2D<I8> { using type = Array2DI8; };
template<> struct Array2D<R4> { using type = Array2DR4; };
template<> struct Array2D<R8> { using type = Array2DR8; };

template<typename T> struct Array3D;
template<> struct Array3D<I4> { using type = Array3DI4; };
template<> struct Array3D<I8> { using type = Array3DI8; };
template<> struct Array3D<R4> { using type = Array3DR4; };
template<> struct Array3D<R8> { using type = Array3DR8; };

// Convenience aliases
template<typename T> 
using Array1D_t = typename Array1D<T>::type;

template<typename T>
using Array2D_t = typename Array2D<T>::type;

template<typename T>
using Array3D_t = typename Array3D<T>::type;

//// Variant-aware overload (in AnalysisOperator.h or similar)
//template <typename DstVariant, typename SrcVariant>
//std::enable_if_t<(is_array_variant_v<DstVariant> && 
//                  is_array_variant_v<SrcVariant>)>
//deepCopyVariant(DstVariant &&Dst, SrcVariant &&Src) {
//   std::visit([](auto &dst, const auto &src) {
//      // This is where the variant→view extraction happens
//      Kokkos::deep_copy(dst, src);  // Actual views, works with Kokkos
//   }, 
//   std::forward<DstVariant>(Dst), 
//   std::forward<SrcVariant>(Src));
//}

//// NEW: Overload for variant-to-variant deep copy
//template <typename DstVariant, typename SrcVariant>
//std::enable_if_t<
//    (std::is_same_v<std::decay_t<DstVariant>, Anlys1DVariant> ||
//     std::is_same_v<std::decay_t<DstVariant>, Anlys2DVariant> ||
//     std::is_same_v<std::decay_t<DstVariant>, Anlys3DVariant> ||
//     std::is_same_v<std::decay_t<DstVariant>, AnlysAnyVariant>) &&
//    (std::is_same_v<std::decay_t<SrcVariant>, Anlys1DVariant> ||
//     std::is_same_v<std::decay_t<SrcVariant>, Anlys2DVariant> ||
//     std::is_same_v<std::decay_t<SrcVariant>, Anlys3DVariant> ||
//     std::is_same_v<std::decay_t<SrcVariant>, AnlysAnyVariant>)>
//deepCopyVariant(DstVariant &&Dst, SrcVariant &&Src) {
//   
//   std::visit([](auto &dstArray, const auto &srcArray) {
//      // Now dstArray and srcArray are actual Kokkos::Views
//      // Kokkos::deep_copy handles scalar-to-array automatically
//      Kokkos::deep_copy(dstArray, srcArray);
//   }, 
//   std::forward<DstVariant>(Dst), 
//   std::forward<SrcVariant>(Src));
//}

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

   /// Returns true if Field has already been computed on this timestamp
   bool isCacheValid(const TimeInstant &TimeStamp);

   /// Initialize operator: create and register output fields in Field map
   virtual void initialize(const Config *Options,
                           const MachEnv *InEnv,
                           const HorzMesh *Mesh,
                           const VertCoord *VCoord) = 0;

   /// Perform computation of Analysis fields. Data arrays of input field
   /// retrieved from Field map using input field names. Writes to
   /// operator-owned output arrays
   virtual void compute(const TimeInstant &TimeStamp) = 0;


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
