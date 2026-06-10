//===-- analysis/Analysis.cpp - analysis module -----------------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//

#include "Analysis.h"
 
namespace OMEGA {

void Analysis::init() {

   registerAllAnalysisOperators();

   AnalysisOrchestrator::init();

}

void Analysis::finalize() {

   AnalysisOrchestrator::clear();

}

} // end namespace OMEGA


//===----------------------------------------------------------------------===//
