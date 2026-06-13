#include "AnalysisOperator.h"

namespace OMEGA {

AnalysisOperator::AnalysisOperator() {

}

const std::string AnalysisOperator::getOperatorType() {
   return OperatorTypeName;
}

const std::string AnalysisOperator::getName() { return InstanceName; }

const std::vector<std::string> AnalysisOperator::getInputFieldNames() {
   return InputNames;
}

const std::vector<std::string> AnalysisOperator::getOutputFieldNames() {
   return OutputNames;
}

bool AnalysisOperator::isCacheValid(const TimeInstant &TimeStamp) {
   bool IsValid = false;
   if (FieldComputed && LastComputed == TimeStamp) {
      IsValid = true;
   }

   return IsValid;
}

} // end namespace OMEGA
