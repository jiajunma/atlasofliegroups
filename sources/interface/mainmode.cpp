/*
  This is mainmode.cpp

  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Lie Groups and Representations

  For copyright and license information see the LICENSE file
*/

#include "mainmode.h"
#include "commands.h"
#include "test.h"     // to absorb test commands

#include "basic_io.h"
#include "complexredgp.h"
#include "complexredgp_io.h"
#include "realredgp.h"
#include "realredgp_io.h"
#include "emptymode.h"
#include "error.h"
#include "helpmode.h"
#include "interactive.h"
#include "io.h"
#include "ioutils.h"
#include "realform_io.h"
#include "realmode.h"
#include "rootdata.h"
#include "kgb.h"
#include "kgb_io.h"
#include "testprint.h"
#include "prettyprint.h"
#include "tags.h"

namespace atlas {


/****************************************************************************

  This file contains the definitions for the "main" command mode, which
  is the central mode for the program.

  Basically, entering the main mode means setting the group type. In an
  interactive session, there is always a "current group"; this may be exported
  through the function "currentComplexGroup". The current group may be changed
  with the "type" command, which amounts to exiting and re-entering the main
  mode.

  NOTE : it could be useful to have several active groups simultaneously, say
  for comparison purposes. This should be easy to do (an easy scheme would
  be for instance to have "first", "second" switch between two groups; but
  more sophisticated things are possible.)

*****************************************************************************/

namespace {

  using namespace mainmode;

  void main_mode_entry() throw(commands::EntryError);
  void main_mode_exit();

  // functions for the predefined commands

  void type_f();
  void cmatrix_f();
  void rootdatum_f();
  void roots_f();
  void coroots_f();
  void simpleroots_f();
  void simplecoroots_f();
  void posroots_f();
  void poscoroots_f();
  void realform_f();
  void showrealforms_f();
  void showdualforms_f();
  void blocksizes_f();
  void gradings_f();
  void strongreal_f();
  void dual_kgb_f();
  void help_f();

  // local variables
  // these have been changed to pointers to avoid swapping of G_C

  ComplexReductiveGroup* G_C_pointer=NULL;
  ComplexReductiveGroup* dual_G_C_pointer=NULL;
  complexredgp_io::Interface* G_I_pointer=NULL;

 }


/*****************************************************************************

        Chapter I -- Functions declared in mainmode.h

******************************************************************************/

namespace mainmode {

ComplexReductiveGroup& currentComplexGroup()

{
  return *G_C_pointer;
}

ComplexReductiveGroup& current_dual_group()
{
  if (dual_G_C_pointer==NULL)
    dual_G_C_pointer = new ComplexReductiveGroup
      (currentComplexGroup(), tags::DualTag());
  return *dual_G_C_pointer;
}

complexredgp_io::Interface& currentComplexInterface()
{
  return *G_I_pointer;
}

void replaceComplexGroup(ComplexReductiveGroup* G
			,complexredgp_io::Interface* I)
{
  delete G_C_pointer;
  delete dual_G_C_pointer;
  delete G_I_pointer;
  G_C_pointer=G;
  dual_G_C_pointer=NULL;
  G_I_pointer=I;

}

} // namespace mainmode

/****************************************************************************

        Chapter II -- The main mode |CommandMode|

  One instance of |CommandMode| for the main mode is created at the
  first call of |mainMode()|; further calls just return a reference to it.

*****************************************************************************/

namespace {

/*
  Synopsis: entry function to the main program mode.

  It attempts to set the group type interactively. Throws an EntryError on
  failure.
*/
void main_mode_entry() throw(commands::EntryError)
{
  try {
    interactive::getInteractive(G_C_pointer,G_I_pointer);
  }
  catch(error::InputError& e) {
    e("complex group not set");
    throw commands::EntryError();
  }
}

// function only called from |commands::exitMode|
void main_mode_exit()
{
  replaceComplexGroup(NULL,NULL);
}

} // namespace


namespace mainmode {

/*
  Synopsis: returns a |CommandMode| object that is constructed on first call.
*/
commands::CommandMode& mainMode()
{
  static commands::CommandMode main_mode
    ("main: ",main_mode_entry,main_mode_exit);
  if (main_mode.empty()) // true on first call
  {
    // add the commands from the empty mode
    commands::addCommands(main_mode,emptymode::emptyMode());

    // add the commands from the current mode
    main_mode.add("type",type_f);
    main_mode.add("cmatrix",cmatrix_f);
    main_mode.add("rootdatum",rootdatum_f);
    main_mode.add("roots",roots_f);
    main_mode.add("coroots",coroots_f);
    main_mode.add("simpleroots",simpleroots_f);
    main_mode.add("simplecoroots",simplecoroots_f);
    main_mode.add("posroots",posroots_f);
    main_mode.add("poscoroots",poscoroots_f);
    main_mode.add("realform",realform_f);
    main_mode.add("showrealforms",showrealforms_f);
    main_mode.add("showdualforms",showdualforms_f);
    main_mode.add("blocksizes",blocksizes_f);
    main_mode.add("gradings",gradings_f);
    main_mode.add("strongreal",strongreal_f);
    main_mode.add("dualkgb",dual_kgb_f); // here, since no real form needed
    main_mode.add("help",help_f); // override
    main_mode.add("q",commands::exitMode);

    // add test commands

    test::addTestCommands(main_mode,MainmodeTag());
  }
  return main_mode;
}

} // namespace mainmode

/*****************************************************************************

        Chapter III --- Functions for the predefined commands

  This section contains the definitions of the functions associated to the
  various commands defined in this mode.

******************************************************************************/

namespace {

// Print the Cartan matrix on stdout.
void cmatrix_f()
{
  prettyprint::printMatrix
    (std::cout,currentComplexGroup().rootDatum().cartanMatrix());

}


// Print information about the root datum (see testprint.cpp for details).
void rootdatum_f()
{
  ioutils::OutputFile file;
  testprint::print(file,currentComplexGroup().rootDatum());
}


// Print the roots in the lattice basis.
void roots_f()
{
  ioutils::OutputFile file;

  const RootDatum& rd = currentComplexGroup().rootDatum();

  WeightList::const_iterator first = rd.beginRoot();
  WeightList::const_iterator last = rd.endRoot();
  basic_io::seqPrint(file,first,last,"\n") << std::endl;
}

// Print the coroots in the lattice basis.
void coroots_f()
{
  ioutils::OutputFile file;

  const RootDatum& rd = currentComplexGroup().rootDatum();

  CoweightList::const_iterator first = rd.beginCoroot();
  CoweightList::const_iterator last = rd.endCoroot();
  basic_io::seqPrint(file,first,last,"\n") << std::endl;
}


// Print the simple roots in the lattice coordinates.
void simpleroots_f()
{
  const RootDatum& rd = currentComplexGroup().rootDatum();

  rootdata::WRootIterator first = rd.beginSimpleRoot();
  rootdata::WRootIterator last = rd.endSimpleRoot();
  basic_io::seqPrint(std::cout,first,last,"\n") << std::endl;
}

// Print the simple coroots in the lattice coordinates.
void simplecoroots_f()
{
  const RootDatum& rd = currentComplexGroup().rootDatum();

  rootdata::WRootIterator first = rd.beginSimpleCoroot();
  rootdata::WRootIterator last = rd.endSimpleCoroot();
  basic_io::seqPrint(std::cout,first,last,"\n") << std::endl;
}

// Print the positive roots in the lattice basis.
void posroots_f()
{
  ioutils::OutputFile file;

  const RootDatum& rd = currentComplexGroup().rootDatum();

  rootdata::WRootIterator first = rd.beginPosRoot();
  rootdata::WRootIterator last = rd.endPosRoot();
  basic_io::seqPrint(file,first,last,"\n") << std::endl;
}

// Print the positive coroots in the lattice basis.
void poscoroots_f()
{
  ioutils::OutputFile file;

  const RootDatum& rd = currentComplexGroup().rootDatum();

  rootdata::WRootIterator first = rd.beginPosCoroot();
  rootdata::WRootIterator last = rd.endPosCoroot();
  basic_io::seqPrint(file,first,last,"\n") << std::endl;
}

void help_f() // override more extensive help of empty mode by simple help
{
  activate(helpmode::helpMode());
}

// Print the matrix of blocksizes.
void blocksizes_f()
{
  complexredgp_io::printBlockSizes(std::cout,currentComplexInterface());
}

// Activates real mode (user will select real form)
void realform_f()
{
  commands::activate(realmode::realMode());
}


void showrealforms_f()
{
  const realform_io::Interface& rfi =
    currentComplexInterface().realFormInterface();

  std::cout << "(weak) real forms are:" << std::endl;
  realform_io::printRealForms(std::cout,rfi);
}

void showdualforms_f()
{
  const realform_io::Interface& rfi =
    currentComplexInterface().dualRealFormInterface();

  std::cout << "(weak) dual real forms are:" << std::endl;
  realform_io::printRealForms(std::cout,rfi);
}


// Print the gradings associated to the weak real forms.
void gradings_f()
{
  ComplexReductiveGroup& G_C = currentComplexGroup();

  // get Cartan class; abort if unvalid
  size_t cn=interactive::get_Cartan_class(G_C.Cartan_set(G_C.quasisplit()));

  ioutils::OutputFile file;

  static_cast<std::ostream&>(file) << std::endl;
  complexredgp_io::printGradings(file,cn,currentComplexInterface())
      << std::endl;

}

// Print information about strong real forms.
void strongreal_f()
{
  ComplexReductiveGroup& G_C = currentComplexGroup();

  // get Cartan class; abort if unvalid
  size_t cn=interactive::get_Cartan_class(G_C.Cartan_set(G_C.quasisplit()));

  ioutils::OutputFile file;
  file << "\n";
  realredgp_io::printStrongReal
    (file,
     mainmode::currentComplexGroup(),
     mainmode::currentComplexInterface().realFormInterface(),
     cn);
}

// Print a kgb table for a dual real form.
void dual_kgb_f()
{
  ComplexReductiveGroup& G_C = currentComplexGroup();

  const complexredgp_io::Interface& G_I = currentComplexInterface();
  const RealFormNbrList rfl = // get list of all dual real forms
    G_C.dualRealFormLabels(G_C.mostSplit(G_C.quasisplit()));

  RealFormNbr drf;

  interactive::getInteractive(drf,G_I,rfl,tags::DualTag());

  // the complex group must be in a variable: is non-const for real group
  ComplexReductiveGroup dG_C(G_C,tags::DualTag());
  RealReductiveGroup dG(dG_C,drf);

  std::cout << "dual kgbsize: " << dG.KGB_size() << std::endl;
  ioutils::OutputFile file;

  KGB kgb(dG,dG.Cartan_set());
  kgb_io::printKGB(file,kgb);
}

/*
  Reset the type, effectively reentering the main mode. If the construction
  of the new type fails, the current type remains in force.
*/
void type_f()
{
  try
  {
    ComplexReductiveGroup* G;
    complexredgp_io::Interface* I;
    interactive::getInteractive(G,I);
    replaceComplexGroup(G,I);
  }
  catch(error::InputError& e) {
    e("complex group not changed");
  }

}

} // namespace

} // namespace atlas
