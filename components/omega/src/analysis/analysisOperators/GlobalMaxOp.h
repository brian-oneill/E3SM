#ifndef OMEGA_GLOBALMAXOP_H
#define OMEGA_GLOBALMAXOP_H


//===-- analysis/analysisOperators/GlobalMaxOp.h ----------------*- C++ -*-===//
///
//===----------------------------------------------------------------------===//

#include "AnalysisOperator.h"
#include "Field.h"
#include "Reductions.h"

namespace OMEGA {

class GlobalMaxOp : public AnalysisOperator {
 public:

   GlobalMaxOp(const std::string &Name, const Config &Options);

   ~GlobalMaxOp() override = default;

   void initialize(const Config *Options,
                   const HorzMesh *Mesh,
                   const VertCoord *VCoord) override;

   void compute(const TimeInstant &ts) override;

 private:

   // Member data
   const HorzMesh *Mesh;                    ///< Horizontal mesh
   const VertCoord *VCoord;                 ///< VertCoord

   /// Output data storage - holds exactly one array type matching input
   std::variant<Array1DR4, Array1DR8, Array1DI4, Array1DI8> OutputData;

};

} // namespace OMEGA

#endif

