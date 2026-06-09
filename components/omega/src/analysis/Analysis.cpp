//===-- analysis/Analysis.cpp - analysis module -----------------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//

#include "Analysis.h"
 
namespace OMEGA {

void Analysis::init(){
   auto DefMesh = HorzMesh::getDefault();
   auto DefVCoord = VertCoord::getDefault();
   auto DefEnv  = MachEnv::getDefault();


   registerAllAnalysisOperators();

   AnalysisOrchestrator::init();

}

} // end namespace OMEGA


//===----------------------------------------------------------------------===//
