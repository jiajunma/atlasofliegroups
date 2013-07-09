/*
  This is complexredgp_io.h

  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Lie Groups and Representations

  For license information see the LICENSE file
*/

#ifndef COMPLEXREDGP_IO_H  /* guard against multiple inclusions */
#define COMPLEXREDGP_IO_H



#include "realform_io.h"	// containment of |Interface|s

namespace atlas {

/******** function declarations *********************************************/

namespace complexredgp_io {

std::ostream& printBlockSizes(std::ostream&, Interface&);

std::ostream& printGradings(std::ostream&, size_t, Interface&);

}

/******** type definitions **************************************************/

namespace complexredgp_io {

class Interface {

 private:

  ComplexReductiveGroup* d_complexGroup;

  realform_io::Interface d_realFormInterface;
  realform_io::Interface d_dualRealFormInterface;

 public:

// constructors and destructors
  Interface(ComplexReductiveGroup&, const lietype::Layout&);

  ~Interface() {}

// copy, assignment and swap
  void swap(Interface&);

// accessors
  const ComplexReductiveGroup& complexGroup() const {
    return *d_complexGroup;
  }

  const realform_io::Interface& dualRealFormInterface() const {
    return d_dualRealFormInterface;
  }

  const realform_io::Interface& realFormInterface() const {
    return d_realFormInterface;
  }

// manipulators
  ComplexReductiveGroup& complexGroup() {
    return *d_complexGroup;
  }

};

}

}

#endif
