#ifndef OMEGA_ANALYSISOP_H
#define OMEGA_ANALYSISOP_H

//===----------------------------------------------------------------------===//

#include "Config.h"
#include "DataTypes.h"
#include "Dimension.h"
#include "Error.h"
#include "Field.h"
#include "HorzMesh.h"
#include "Logging.h"
#include "MachEnv.h"
#include "OmegaDispatch.h"
#include "OmegaKokkos.h"
#include "TimeMgr.h"
#include "VertCoord.h"

#include <string>

namespace OMEGA {

/// Temporal operators have an accumulation phase and a operation/output phase
enum class TemporalPhase {Accumulate, Operate};

/// Base case: return the config
inline Config makeOpConfig() {
    return Config();
}

/// Create a Config with key-value pairs
/// Usage: makeOpConfig({"key1", value1}, {"key2", value2}, ...)
template<typename T, typename... Args>
Config makeOpConfig(const std::pair<std::string, T>& Param, Args... OtherArgs) {
    Config Cfg = makeOpConfig(OtherArgs...);  // Recurse to build from end
    Cfg.add(Param.first, Param.second);
    return Cfg;
}

///
template<typename T>
using OpParam = std::pair<std::string, std::decay_t<T>>;

///
template<typename T>
OpParam<T> opParam(std::string Key, T&& Value) {
    return {
        std::move(Key),
        std::forward<T>(Value)
    };
}

/// The AnalysisOperator class ...
class AnalysisOperator {

 public:
   AnalysisOperator();
   AnalysisOperator(const std::string &OperatorType);


   ~AnalysisOperator();

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

   /// Initialize operator 
   virtual void initialize(
       const MachEnv *Env,
       const HorzMesh *Mesh,
       const VertCoord *VCoord,
       Config Options
   );

   /// Perform computation of Analysis fields. Data arrays of input field
   /// retrieved from Field map using input field names. Writes to
   /// operator-owned output arrays
   virtual void compute(const TimeInstant &TimeStamp) = 0;


 protected:

   // Member data
   const HorzMesh *Mesh;                    ///< Horizontal mesh
   const VertCoord *VCoord;                 ///< VertCoord
   MPI_Comm Comm;


   std::string OperatorTypeName;
   std::string InstanceName;
   std::vector<std::string> InputNames;
   std::vector<std::string> OutputNames;

 
   TimeInstant LastComputed;
   bool FieldComputed;
}; // end class AnalysisOperator

} // end namespace OMEGA

#endif
