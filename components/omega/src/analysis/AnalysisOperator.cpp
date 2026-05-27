#include "AnalysisOperator.h"

namespace OMEGA {

std::string AnalysisOperator::getOperatorType() const {
   return OperatorTypeName;
}

std::string AnalysisOperator::getName() const { return InstanceName; }

std::vector<std::string> AnalysisOperator::getInputFieldNames() const {
   return InputNames;
}

std::vector<std::string> AnalysisOperator::getOutputFieldNames() const {
   return OutputNames;
}


} // end namespace OMEGA
