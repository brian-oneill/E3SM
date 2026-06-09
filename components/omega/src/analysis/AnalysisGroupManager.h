#ifndef OMEGA_ANALYSISGROUPMANAGER_H
#define OMEGA_ANALYSISGROUPMANAGER_H

#include "AnalysisGroup.h"
#include <memory>
#include <vector>

namespace OMEGA {

class AnalysisGroupManager {
 public:

   static void initialize();
   static void finalize();

 private:

   static std::vector<std::unique_ptr<AnalysisGroup>> AllGroups;

};

} // end namespace OMEGA

#endif
