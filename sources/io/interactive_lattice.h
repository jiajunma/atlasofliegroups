/*
  This is interactive_lattice.h

  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Lie Groups and Representations

  For license information see the LICENSE file
*/

#ifndef INTERACTIVE_LATTICE_H  /* guard against multiple inclusions */
#define INTERACTIVE_LATTICE_H


#include "error.h"
#include "atlas_types.h"

/******** function declarations **********************************************/

namespace atlas {

namespace interactive_lattice {

  int getGenerators(RatWeightList&,
		    const CoeffList&)
    throw(error::InputError);

  int getLattice(const CoeffList&, WeightList&)
    throw(error::InputError);

}

}

#endif
