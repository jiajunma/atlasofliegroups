/*
  This is complexredgp_io.h
  
  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups version 0.2.3 

  See file main.cpp for full copyright notice
*/

#ifndef COMPLEXREDGP_IO_H  /* guard against multiple inclusions */
#define COMPLEXREDGP_IO_H

#include "complexredgp_io_fwd.h"

#include "complexredgp_fwd.h"
#include "latticetypes_fwd.h"
#include "layout.h"
#include "lietype_fwd.h"

#include "realform_io.h"

namespace atlas {

/******** function declarations *********************************************/

namespace complexredgp_io {

std::ostream& printBlockSizes(std::ostream&, const Interface&);

std::ostream& printCartanClass(std::ostream&, size_t, const Interface&);

std::ostream& printGradings(std::ostream&, size_t, const Interface&);

}

/******** type definitions **************************************************/

namespace complexredgp_io {

class Interface {

 private:

  complexredgp::ComplexReductiveGroup* d_complexGroup;

  layout::Layout d_layout;
  realform_io::Interface d_realFormInterface;
  realform_io::Interface d_dualRealFormInterface;

 public:

// constructors and destructors
  Interface():d_complexGroup(0) {}

  explicit Interface(complexredgp::ComplexReductiveGroup& G)
    :d_complexGroup(&G) {}

  Interface(complexredgp::ComplexReductiveGroup&, const layout::Layout&);

  ~Interface() {}

// copy, assignment and swap
  void swap(Interface&);

// accessors
  const complexredgp::ComplexReductiveGroup& complexGroup() const {
    return *d_complexGroup;
  }

  const realform_io::Interface& dualRealFormInterface() const {
    return d_dualRealFormInterface;
  }

  const layout::Layout& layout() const {
    return d_layout;
  }

  const realform_io::Interface& realFormInterface() const {
    return d_realFormInterface;
  }

// manipulators
  complexredgp::ComplexReductiveGroup& complexGroup() {
    return *d_complexGroup;
  }

};

}

}

#endif
