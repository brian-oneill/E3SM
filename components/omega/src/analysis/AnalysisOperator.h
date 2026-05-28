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
