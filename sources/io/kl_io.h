/*
  This is kl_io.h

  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Lie Groups and Representations

  For license information see the LICENSE file
*/

#ifndef KL_IO_H  /* guard against multiple inclusions */
#define KL_IO_H

#include <iosfwd>

#include "atlas_types.h"

namespace atlas {

/******** function declarations *********************************************/

namespace kl_io {

  std::ostream& printAllKL
    (std::ostream&, const kl::KLContext&, Block&);
  std::ostream& printPrimitiveKL
    (std::ostream&, const kl::KLContext&, Block&);

  std::ostream& printKLList(std::ostream&, const kl::KLContext&);

  std::ostream& printMu(std::ostream&, const kl::KLContext&);

}

}

#endif
