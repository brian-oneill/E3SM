//===-- Test driver for OMEGA Anlysis Module ---- ----------------*- C++ -*-===/
//
//===-----------------------------------------------------------------------===/

#include "Analysis.h"

#include "Pacer.h"
#include <cmath>

using namespace OMEGA;


int main(int argc, char *argv[]) {

   // Initialize error code
   Error ErrAll;

   MPI_Init(&argc, &argv);
   Kokkos::initialize();
   Pacer::initialize(MPI_COMM_WORLD);
   Pacer::setPrefix("Omega:");
   {

   }

   Pacer::finalize();
   Kokkos::finalize();
   MPI_Finalize();

   return 0;

};
