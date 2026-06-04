#ifndef OMEGA_DISPATCH_H
#define OMEGA_DISPATCH_H

// A Field stores its attached Kokkos View through a type-erased shared_ptr<void>.
// This allows fields with different scalar types, ranks, and memory spaces to
// live in one registry, but it also means that after lookup by name the concrete
// C++ type of the array is no longer known at compile time. Since Kokkos kernels
// and templated array utilities require the concrete View type, we need an
// explicit dispatch step whenever code starts from a runtime Field and needs to
// operate on the attached data.
//
// dispatchFieldArray uses the Field metadata -- ArrayDataType, number of
// dimensions, and ArrayMemLoc -- to select the corresponding OMEGA array alias
// such as Array2DR8 or HostArray1DI4. It then retrieves the typed shallow-copy
// Kokkos View with getDataArray<ArrayT>() and calls a templated operation object
// as op.apply<ArrayT>(array). The dispatch happens on the host before any Kokkos
// kernel launch; device code only sees the concrete View type selected by the
// dispatcher. This keeps type-erased registry lookup centralized while preserving
// normal compile-time Kokkos specialization inside the operation implementation.

#include "Field.h"

namespace OMEGA {

#define OMEGA_FIELD_ARRAY_TYPES(X)                                      \
   /* Both R8: same aliases as Array... */                              \
   X(ArrayDataType::R8, 1, ArrayMemLoc::Both, Array1DR8)                \
   X(ArrayDataType::R8, 2, ArrayMemLoc::Both, Array2DR8)                \
   X(ArrayDataType::R8, 3, ArrayMemLoc::Both, Array3DR8)                \
   X(ArrayDataType::R8, 4, ArrayMemLoc::Both, Array4DR8)                \
   X(ArrayDataType::R8, 5, ArrayMemLoc::Both, Array5DR8)
//                                                                        \
//   /* Device I4 */                                                      \
//   X(ArrayDataType::I4, 1, ArrayMemLoc::Device, Array1DI4)              \
//   X(ArrayDataType::I4, 2, ArrayMemLoc::Device, Array2DI4)              \
//   X(ArrayDataType::I4, 3, ArrayMemLoc::Device, Array3DI4)              \
//   X(ArrayDataType::I4, 4, ArrayMemLoc::Device, Array4DI4)              \
//   X(ArrayDataType::I4, 5, ArrayMemLoc::Device, Array5DI4)              \
//                                                                        \
//   /* Device I8 */                                                      \
//   X(ArrayDataType::I8, 1, ArrayMemLoc::Device, Array1DI8)              \
//   X(ArrayDataType::I8, 2, ArrayMemLoc::Device, Array2DI8)              \
//   X(ArrayDataType::I8, 3, ArrayMemLoc::Device, Array3DI8)              \
//   X(ArrayDataType::I8, 4, ArrayMemLoc::Device, Array4DI8)              \
//   X(ArrayDataType::I8, 5, ArrayMemLoc::Device, Array5DI8)              \
//                                                                        \
//   /* Device R4 */                                                      \
//   X(ArrayDataType::R4, 1, ArrayMemLoc::Device, Array1DR4)              \
//   X(ArrayDataType::R4, 2, ArrayMemLoc::Device, Array2DR4)              \
//   X(ArrayDataType::R4, 3, ArrayMemLoc::Device, Array3DR4)              \
//   X(ArrayDataType::R4, 4, ArrayMemLoc::Device, Array4DR4)              \
//   X(ArrayDataType::R4, 5, ArrayMemLoc::Device, Array5DR4)              \
//                                                                        \
//   /* Device R8 */                                                      \
//   X(ArrayDataType::R8, 1, ArrayMemLoc::Device, Array1DR8)              \
//   X(ArrayDataType::R8, 2, ArrayMemLoc::Device, Array2DR8)              \
//   X(ArrayDataType::R8, 3, ArrayMemLoc::Device, Array3DR8)              \
//   X(ArrayDataType::R8, 4, ArrayMemLoc::Device, Array4DR8)              \
//   X(ArrayDataType::R8, 5, ArrayMemLoc::Device, Array5DR8)              \
//                                                                        \
//   /* Host I4 */                                                        \
//   X(ArrayDataType::I4, 1, ArrayMemLoc::Host, HostArray1DI4)            \
//   X(ArrayDataType::I4, 2, ArrayMemLoc::Host, HostArray2DI4)            \
//   X(ArrayDataType::I4, 3, ArrayMemLoc::Host, HostArray3DI4)            \
//   X(ArrayDataType::I4, 4, ArrayMemLoc::Host, HostArray4DI4)            \
//   X(ArrayDataType::I4, 5, ArrayMemLoc::Host, HostArray5DI4)            \
//                                                                        \
//   /* Host I8 */                                                        \
//   X(ArrayDataType::I8, 1, ArrayMemLoc::Host, HostArray1DI8)            \
//   X(ArrayDataType::I8, 2, ArrayMemLoc::Host, HostArray2DI8)            \
//   X(ArrayDataType::I8, 3, ArrayMemLoc::Host, HostArray3DI8)            \
//   X(ArrayDataType::I8, 4, ArrayMemLoc::Host, HostArray4DI8)            \
//   X(ArrayDataType::I8, 5, ArrayMemLoc::Host, HostArray5DI8)            \
//                                                                        \
//   /* Host R4 */                                                        \
//   X(ArrayDataType::R4, 1, ArrayMemLoc::Host, HostArray1DR4)            \
//   X(ArrayDataType::R4, 2, ArrayMemLoc::Host, HostArray2DR4)            \
//   X(ArrayDataType::R4, 3, ArrayMemLoc::Host, HostArray3DR4)            \
//   X(ArrayDataType::R4, 4, ArrayMemLoc::Host, HostArray4DR4)            \
//   X(ArrayDataType::R4, 5, ArrayMemLoc::Host, HostArray5DR4)            \
//                                                                        \
//   /* Host R8 */                                                        \
//   X(ArrayDataType::R8, 1, ArrayMemLoc::Host, HostArray1DR8)            \
//   X(ArrayDataType::R8, 2, ArrayMemLoc::Host, HostArray2DR8)            \
//   X(ArrayDataType::R8, 3, ArrayMemLoc::Host, HostArray3DR8)            \
//   X(ArrayDataType::R8, 4, ArrayMemLoc::Host, HostArray4DR8)            \
//   X(ArrayDataType::R8, 5, ArrayMemLoc::Host, HostArray5DR8)            \
//                                                                        \
//   /* Both I4: same aliases as Array... */                              \
//   X(ArrayDataType::I4, 1, ArrayMemLoc::Both, Array1DI4)                \
//   X(ArrayDataType::I4, 2, ArrayMemLoc::Both, Array2DI4)                \
//   X(ArrayDataType::I4, 3, ArrayMemLoc::Both, Array3DI4)                \
//   X(ArrayDataType::I4, 4, ArrayMemLoc::Both, Array4DI4)                \
//   X(ArrayDataType::I4, 5, ArrayMemLoc::Both, Array5DI4)                \
//                                                                        \
//   /* Both I8: same aliases as Array... */                              \
//   X(ArrayDataType::I8, 1, ArrayMemLoc::Both, Array1DI8)                \
//   X(ArrayDataType::I8, 2, ArrayMemLoc::Both, Array2DI8)                \
//   X(ArrayDataType::I8, 3, ArrayMemLoc::Both, Array3DI8)                \
//   X(ArrayDataType::I8, 4, ArrayMemLoc::Both, Array4DI8)                \
//   X(ArrayDataType::I8, 5, ArrayMemLoc::Both, Array5DI8)                \
//                                                                        \
//   /* Both R4: same aliases as Array... */                              \
//   X(ArrayDataType::R4, 1, ArrayMemLoc::Both, Array1DR4)                \
//   X(ArrayDataType::R4, 2, ArrayMemLoc::Both, Array2DR4)                \
//   X(ArrayDataType::R4, 3, ArrayMemLoc::Both, Array3DR4)                \
//   X(ArrayDataType::R4, 4, ArrayMemLoc::Both, Array4DR4)                \
//   X(ArrayDataType::R4, 5, ArrayMemLoc::Both, Array5DR4)                \
//                                                                        \
//   /* Both R8: same aliases as Array... */                              \
//   X(ArrayDataType::R8, 1, ArrayMemLoc::Both, Array1DR8)                \
//   X(ArrayDataType::R8, 2, ArrayMemLoc::Both, Array2DR8)                \
//   X(ArrayDataType::R8, 3, ArrayMemLoc::Both, Array3DR8)                \
//   X(ArrayDataType::R8, 4, ArrayMemLoc::Both, Array4DR8)                \
//   X(ArrayDataType::R8, 5, ArrayMemLoc::Both, Array5DR8)

template <typename Op>
decltype(auto) dispatchFieldArray(Field &field, Op &&op) {
#define TRY_ARRAY_TYPE(dtype, rank, memloc, ArrayT)                     \
   if (field.getType() == dtype &&                                      \
       field.getNumDims()  == rank &&                                   \
       field.getMemoryLocation()   == memloc) {                         \
      return std::forward<Op>(op)(field.getDataArray<ArrayT>());        \
   }

   OMEGA_FIELD_ARRAY_TYPES(TRY_ARRAY_TYPE)

#undef TRY_ARRAY_TYPE
   ABORT_ERROR("Unsupported array type/rank/location combination for field {}",
               field.getName());
}
#undef OMEGA_FIELD_ARRAY_TYPES


} // end namespace OMEGA
#endif
