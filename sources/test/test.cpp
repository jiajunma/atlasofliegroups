/*
  This is test.cpp

  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Lie Groups and Representations

  For license information see the LICENSE file
*/

#include "io.h"    //needed for help commands

#include "test.h"

#include <cassert>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>

#include "atlas_types.h" // here to preempt double inclusion of _fwd files

#include "free_abelian.h"
#include "permutations.h"
#include "matreduc.h"

#include "dynkin.h"
#include "prerootdata.h"
#include "rootdata.h"
#include "cartanclass.h"
#include "complexredgp.h"
#include "realredgp.h"

#include "kgb.h"
#include "blocks.h"
#include "klsupport.h"
#include "kl.h"
#include "standardrepk.h"
#include "repr.h"

#include "ioutils.h"
#include "basic_io.h"
#include "hkl.h"
#include "prettyprint.h"
#include "interactive.h"
#include "realform_io.h"
#include "kgb_io.h"
#include "block_io.h"

#include "commands.h"
#include "helpmode.h"
#include "emptymode.h"
#include "mainmode.h"
#include "realmode.h"
#include "blockmode.h"
#include "reprmode.h"

#include "testrun.h"
#include "kltest.h"

/*****************************************************************************

  This module contains some commands for testing the program.

******************************************************************************/

namespace atlas {

namespace {
  using namespace test;

  // functions for the test commands

  void test_f();

  void roots_rootbasis_f();
  void coroots_rootbasis_f();
  void posroots_rootbasis_f();
  void poscoroots_rootbasis_f();
  void checkbasept_f();
  void sub_KGB_f();
  void trivial_f();
  void Ktypeform_f();
  void qKtypeform_f();
  void Ktypemat_f();
  void qKtypemat_f();
  void mod_lattice_f();
  void branch_f();
  void qbranch_f();
  void srtest_f();
  void testrun_f();
  void exam_f();

  void X_f();
  void embedding_f();

  void hblock_f();
  void hkllist_f();
  void hklbasis_f();
  void hkltest_f();

  // help functions

  // no |test_h|, test command remains development-reserved; uses |nohelp_h|

  void roots_rootbasis_h();
  void coroots_rootbasis_h();
  void posroots_rootbasis_h();
  void poscoroots_rootbasis_h();
  void mod_lattice_h();
  void checkbasept_h();
  void sub_KGB_h();
  void trivial_h();
  void Ktypeform_h();
  void Ktypemat_h();
  void branch_h();
  void srtest_h();
  void testrun_h();
  void exam_h();

  void nblock_h();

  // tags
  const char* test_tag = "test command (for development only)";

  const char* roots_rootbasis_tag = "outputs the roots in the simple rootbasis";
  const char* coroots_rootbasis_tag =
    "outputs the coroots in the simple coroot basis";
  const char* posroots_rootbasis_tag =
     "outputs the positive roots in the simple root basis";
  const char* poscoroots_rootbasis_tag =
      "outputs the positive coroots in the simple coroot basis";
  const char* sub_KGB_tag =
    "computes subset of KGB data used in Ktypeform";
  const char* trivial_tag =
    "tries to compute Ktypeform for trivial representation";
  const char* Ktypeform_tag = "computes formula for a K-type";
  const char* Ktypemat_tag =
     "computes matrix relating K-types and standard modules";
  const char* mod_lattice_tag =
    "gives a basis of quotient of character lattice";
  const char* branch_tag = "computes restriction of representation to K";
  const char* srtest_tag = "gives information about a representation";
  const char* testrun_tag =
                "iterates over root data of given rank, calling examine";
  const char* examine_tag = "tests whether block is ordered by Weyl length";

  const char* nblock_tag = "computes a non-integral block";
  const char* partial_block_tag = "computes part of a non-integral block";

/*
  For convenience, the "test" command is added to the mode that is flagged by
  the testMode constant defined here; therefore "test" appears (conditionally)
  in every template instance of |addTestCommands| and of |addTestHelp| below.
  Set this constant according to the requirements of the |test_f| function.
*/
  enum TestMode {EmptyMode, MainMode, RealMode, BlockMode, ReprMode,
		 numTestMode};
  const TestMode testMode = RealMode; // currently does subsystem KGB test

  // utilities
  const RootDatum& currentRootDatum();

}

/*****************************************************************************

        Chapter I -- Functions declared by test.h

  This section defines the functions declared in test.h :

    - addTestCommands() : adds the test commands to the main command
      tree;
    - addTestHelp() : adds help functionality;

******************************************************************************/

namespace test {

/* |addTestCommands| is a template only so that its declaration is shorter;
   its defining instances are defined just as overloaded functions would,
   since each of them needs to test a specific value of |testMode|.
*/


// Add to the empty mode the test commands that require that mode.
template<>
void addTestCommands<emptymode::EmptymodeTag>
  (commands::CommandMode& mode, emptymode::EmptymodeTag)
{
  mode.add("testrun",testrun_f);
  if (testMode == EmptyMode)
    mode.add("test",test_f);

}


// Add to the main mode the test commands that require that mode.
template<>
void addTestCommands<mainmode::MainmodeTag>
  (commands::CommandMode& mode, mainmode::MainmodeTag)
{
  if (testMode == MainMode)
    mode.add("test",test_f);

  // add additional commands here :

  mode.add("roots_rootbasis",roots_rootbasis_f);
  mode.add("posroots_rootbasis",posroots_rootbasis_f);
  mode.add("coroots_rootbasis",coroots_rootbasis_f);
  mode.add("poscoroots_rootbasis",poscoroots_rootbasis_f);

  mode.add("X",X_f);
}


// Add to the real mode the test commands that require that mode.
template<>
void addTestCommands<realmode::RealmodeTag>
  (commands::CommandMode& mode, realmode::RealmodeTag)
{
  if (testMode == RealMode)
    mode.add("test",test_f);

  // add additional commands here:

  mode.add("checkbasept",checkbasept_f);
  mode.add("sub_KGB",sub_KGB_f);
  mode.add("trivial",trivial_f);
  mode.add("Ktypeform",Ktypeform_f);
  mode.add("qKtypeform",qKtypeform_f);
  mode.add("Ktypemat",Ktypemat_f);
  mode.add("qKtypemat",qKtypemat_f);
  mode.add("mod_lattice",mod_lattice_f);
  mode.add("branch",branch_f);
  mode.add("qbranch",qbranch_f);
  mode.add("srtest",srtest_f);
  mode.add("examine",exam_f);
  mode.add("embedding",embedding_f);
}

// Add to the block mode the test commands that require that mode.
template<>
void addTestCommands<blockmode::BlockmodeTag>
  (commands::CommandMode& mode, blockmode::BlockmodeTag)
{
  if (testMode == BlockMode)
    mode.add("test",test_f);
// Add additional commands here:

  mode.add("hblock",hblock_f);
  mode.add("hkllist",hkllist_f);
  mode.add("hklbasis",hklbasis_f);
  mode.add("hkltest",hkltest_f);
}

// Add to the repr mode the test commands that require that mode.
template<>
void addTestCommands<reprmode::ReprmodeTag>
  (commands::CommandMode& mode, reprmode::ReprmodeTag)
{
  if (testMode == ReprMode)
    mode.add("test",test_f);

  // add additional commands here :
}



// Add to the help mode the test commands that require that mode.
template<> void addTestHelp<emptymode::EmptymodeTag>
             (commands::CommandMode& mode, commands::TagDict& t,
	      emptymode::EmptymodeTag)
{
  using namespace commands;
  using namespace helpmode;

  if (testMode == EmptyMode) {
    mode.add("test",nohelp_h);
    commands::insertTag(t,"test",test_tag);
  }


  // add additional help commands here:
  mode.add("testrun",helpmode::nohelp_h);

  // add additional command tags here:
  commands::insertTag(t,"testrun",testrun_tag);

}


// Add to the main mode the help commands for test commands with that mode
template<> void addTestHelp<mainmode::MainmodeTag>
             (commands::CommandMode& mode, commands::TagDict& t,
	      mainmode::MainmodeTag)
{
  using namespace commands;
  using namespace helpmode;

  if (testMode == MainMode) {
    mode.add("test",nohelp_h);
    insertTag(t,"test",test_tag);
  }

  // add additional help commands here:

  mode.add("roots_rootbasis",roots_rootbasis_h);
  mode.add("posroots_rootbasis",posroots_rootbasis_h);
  mode.add("coroots_rootbasis",coroots_rootbasis_h);
  mode.add("poscoroots_rootbasis",poscoroots_rootbasis_h);

  // add additional command tags here :

  insertTag(t,"roots_rootbasis",roots_rootbasis_tag);
  insertTag(t,"posroots_rootbasis",posroots_rootbasis_tag);
  insertTag(t,"coroots_rootbasis",coroots_rootbasis_tag);
  insertTag(t,"poscoroots_rootbasis",poscoroots_rootbasis_tag);

}


// Add to the real mode the help commands for test commands with that mode
template<> void addTestHelp<realmode::RealmodeTag>
             (commands::CommandMode& mode, commands::TagDict& t,
	      realmode::RealmodeTag)
{
  using namespace commands;
  using namespace helpmode;

  if (testMode == RealMode) {
    mode.add("test",nohelp_h);
    insertTag(t,"test",test_tag);
  }

  // add additional help commands here:

  mode.add("checkbasept",checkbasept_h);
  mode.add("sub_KGB",helpmode::nohelp_h);
  mode.add("trivial",helpmode::nohelp_h);
  mode.add("Ktypeform",Ktypeform_h);
  mode.add("Ktypemat",Ktypemat_h);
  mode.add("mod_lattice",mod_lattice_h);
  mode.add("branch",branch_h);
  mode.add("srtest",srtest_h);
  mode.add("examine",helpmode::nohelp_h);
  mode.add("nblock",nblock_h);


  // add additional command tags here :

  insertTag(t,"sub_KGB",sub_KGB_tag);
  insertTag(t,"trivial",trivial_tag);
  insertTag(t,"Ktypeform",Ktypeform_tag);
  insertTag(t,"Ktypemat",Ktypemat_tag);
  insertTag(t,"mod_lattice",mod_lattice_tag);
  insertTag(t,"branch",branch_tag);
  insertTag(t,"srtest",srtest_tag);
  insertTag(t,"examine",examine_tag);
  insertTag(t,"nblock",nblock_tag);
  insertTag(t,"partial_block",partial_block_tag);

}

// Add to the block mode the help commands for test commands with that mode
template<> void addTestHelp<blockmode::BlockmodeTag>
             (commands::CommandMode& mode, commands::TagDict& t,
	      blockmode::BlockmodeTag)
{
  using namespace commands;
  using namespace helpmode;

  if (testMode == BlockMode) {
    mode.add("test",nohelp_h);
    insertTag(t,"test",test_tag);
  }

  // add additional help commands here:



  // add additional command tags here:

}

} // namespace test


/*****************************************************************************

        Chapter II -- Functions for the test commands

******************************************************************************/

namespace {

  // Main mode functions

// Print the roots in the simple root coordinates.
void roots_rootbasis_f()
{
  const RootSystem& rs =  mainmode::currentComplexGroup().rootSystem();
  ioutils::OutputFile file;

  for (RootNbr i=0; i<rs.numRoots(); ++i)
    prettyprint::printInRootBasis(file,i,rs) << std::endl;
}

// Print the positive roots in the simple root coordinates.
void posroots_rootbasis_f()

{
  const RootSystem& rs = mainmode::currentComplexGroup().rootSystem();

  ioutils::OutputFile file;
  prettyprint::printInRootBasis(file,rs.posRootSet(),rs);
}

// Print the coroots in the simple coroot coordinates.
void coroots_rootbasis_f()
{
  const RootSystem rs (mainmode::currentComplexGroup().dualRootSystem());

  ioutils::OutputFile file;
  for (RootNbr i=0; i<rs.numRoots(); ++i)
    prettyprint::printInRootBasis(file,i,rs) << std::endl;

}

// Print the positive coroots in the simple coroot coordinates.
void poscoroots_rootbasis_f()
{
  const RootSystem rs (mainmode::currentComplexGroup().dualRootSystem());

  ioutils::OutputFile file;
  prettyprint::printInRootBasis(file,rs.posRootSet(),rs);
}

// Real mode functions

void checkbasept_f()
{
  RealReductiveGroup& G_R = realmode::currentRealGroup();

  KGB kgb(G_R,G_R.Cartan_set());
  kltest::checkBasePoint(kgb);
}

void sub_KGB_f()
{
  RealReductiveGroup& G = realmode::currentRealGroup();
  standardrepk::KhatContext khc(G);

  StandardRepK sr=interactive::get_standardrep(khc);

  WeylWord ww;
  standardrepk::PSalgebra p= khc.theta_stable_parabolic(sr,ww);
  KGBEltList sub=khc.sub_KGB(p);

  std::cout << "Conjugating word [" << ww << "]\n";
  kgb_io::print_sub_KGB(std::cout,G.kgb(),sub);
}

void trivial_f()
{
  RealReductiveGroup& G = realmode::currentRealGroup();
  const RootDatum& rd=G.rootDatum();
  const KGB& kgb = G.kgb();

  standardrepk::KhatContext khc(G);

  KGBElt last=kgb.size()-1;

  WeylWord ww;
  standardrepk::PSalgebra q=
    khc.theta_stable_parabolic(khc.KGB_elt_rep(last),ww);

  KGBEltList subset=khc.sub_KGB(q);
  size_t max_l=kgb.length(subset.back());

  standardrepk::combination sum(khc.height_order());
  for (size_t i=0; i<subset.size(); ++i)
  {
    KGBElt x=subset[i];
    StandardRepK sr=khc.std_rep(rd.twoRho(),kgb.titsElt(x));
    standardrepk::combination c=khc.standardize(sr);
    if ((max_l-kgb.length(x))%2 == 0)
      sum += c;
    else
      sum-=c;
  }


  {
    std::ostringstream s; khc.print(s,sum);
    ioutils::foldLine(std::cout,s.str(),"+\n- ","",1) << std::endl;
  }

} // trivial

void Ktypeform_f()
{
  RealReductiveGroup& G = realmode::currentRealGroup();

  standardrepk::KhatContext khc(G);

  StandardRepK sr=interactive::get_standardrep(khc);

  {
    size_t witness;
    if (not khc.isFinal(sr,witness))
    {
      khc.print(std::cout << "Representation ",sr)
        << " is not final, as witnessed by coroot "
	<< G.rootDatum().coroot(khc.fiber(sr).simpleReal(witness)) << ".\n";
      return;
    }
  }

  ioutils::OutputFile f;

  standardrepk::CharForm kf= khc.K_type_formula(sr);

  khc.print(f << "K-type formula for mu(",kf.first) << "):\n";
  {
    std::ostringstream s; khc.print(s,kf.second);
    ioutils::foldLine(f,s.str(),"+\n- ","",1) << std::endl;
  }

  for (size_t i=0; i<khc.nr_reps(); ++i)
    khc.print(f << 'R' << i << ": ",khc.rep_no(i))
      << ", height: " << khc.height(i) << std::endl;


  standardrepk::combination sum(khc.height_order());

  for (standardrepk::Char::const_iterator
	 it=kf.second.begin(); it!=kf.second.end(); ++it)
  {
#ifdef VERBOSE
    khc.print(f,it->first) << " has height " << khc.height(it->first)
				   << std::endl;
    size_t old_size=khc.nr_reps();
#endif
    standardrepk::combination st=khc.standardize(it->first);
#ifdef VERBOSE
    for (size_t i=old_size; i<khc.nr_reps(); ++i)
      khc.print(f << 'R' << i << ": ",khc.rep_no(i))
        << ", height: " << khc.height(i) << std::endl;

    std::ostringstream s; khc.print(s,it->first) << " = ";
    khc.print(s,st,true);
    ioutils::foldLine(f,s.str(),"+\n- ","",1) << std::endl;
#endif
    sum.add_multiple(st,it->second);
  }

  f << "Converted to Standard normal final limit form:\n";
  {
    std::ostringstream s; khc.print(s,sum);
    ioutils::foldLine(f,s.str(),"+\n- ","",1) << std::endl;
  }
} // |Kypeform_f|

void qKtypeform_f()
{
  RealReductiveGroup& G = realmode::currentRealGroup();

  standardrepk::qKhatContext khc(G);

  StandardRepK sr=interactive::get_standardrep(khc);

  {
    size_t witness;
    if (not khc.isFinal(sr,witness))
    {
      khc.print(std::cout << "Representation ",sr)
        << " is not final, as witnessed by coroot "
	<< G.rootDatum().coroot(khc.fiber(sr).simpleReal(witness)) << ".\n";
      return;
    }
  }

  ioutils::OutputFile f;

  standardrepk::q_CharForm kf= khc.q_K_type_formula(sr);

  khc.print(f << "q-K-type formula for mu(",kf.first) << "):\n";
  {
    std::ostringstream s; khc.print(s,kf.second);
    ioutils::foldLine(f,s.str(),"+\n- ","",1) << std::endl;
  }

  standardrepk::q_combin sum(khc.height_order());

  for (standardrepk::q_Char::const_iterator
	 it=kf.second.begin(); it!=kf.second.end(); ++it)
  {
#ifdef VERBOSE
    khc.print(f,it->first) << " has height " << khc.height(it->first)
				   << std::endl;
    size_t old_size=khc.nr_reps();
#endif
    standardrepk::q_combin st=khc.standardize(it->first);
#ifdef VERBOSE
    for (size_t i=old_size; i<khc.nr_reps(); ++i)
      khc.print(f << 'R' << i << ": ",khc.rep_no(i))
        << ", height: " << khc.height(i) << std::endl;

    std::ostringstream s; khc.print(s,it->first) << " = ";
    khc.print(s,st,true);
    ioutils::foldLine(f,s.str(),"+\n- ","",1) << std::endl;
#endif
    sum.add_multiple(st,it->second);
  }

  f << "Converted to Standard normal final limit form:\n";
  {
    std::ostringstream s; khc.print(s,sum);
    ioutils::foldLine(f,s.str(),"+\n- ","",1) << std::endl;
  }
} // |qKypeform_f|

void Ktypemat_f()
{
  RealReductiveGroup& G = realmode::currentRealGroup();

  standardrepk::KhatContext khc(G);

  StandardRepK sr=interactive::get_standardrep(khc);
  khc.normalize(sr);

  {
    size_t witness;
    if (not khc.isStandard(sr,witness))
    {
      khc.print(std::cout << "Representation ",sr)
        << " is not standard, as witnessed by coroot "
	<< G.rootDatum().coroot(khc.fiber(sr).simpleImaginary(witness))
	<< ".\n";
      return;
    }
    if (not khc.isFinal(sr,witness))
    {
      khc.print(std::cout << "Representation ",sr)
        << " is not final, as witnessed by coroot "
	<< G.rootDatum().coroot(khc.fiber(sr).simpleReal(witness)) << ".\n";
      return;
    }
  }

  standardrepk::combination c=khc.standardize(sr);

  if (c.empty())
  {
    khc.print(std::cout << "Representation ",sr) << " is zero.\n";
    return;
  }

  assert(c.size()==1);
  assert(khc.rep_no(c.begin()->first)==sr);

  khc.print(std::cout << "Height of representation ",sr) << " is "
    << khc.height(c.begin()->first) << ".\n";
  unsigned long bound=
    interactive::get_bounded_int(interactive::common_input(),
				 "Give height bound: ",
				 9999);

  ioutils::OutputFile f;

  std::set<standardrepk::equation> singleton;
  singleton.insert(khc.mu_equation(c.begin()->first,bound));

  {
    standardrepk::equation init=*singleton.begin();
    khc.print(f << "Initial formula: mu(",khc.rep_no(init.first))
      << ") =\n";
    {
      std::ostringstream s; khc.print(s,init.second);
      ioutils::foldLine(f,s.str(),"+\n- ","",1) << std::endl;
    }
  }
  std::vector<standardrepk::seq_no> new_order;

#ifdef VERBOSE
  matrix::Matrix_base<standardrepk::CharCoeff> m;
  matrix::Matrix_base<standardrepk::CharCoeff> ktypemat =
    khc.K_type_matrix(singleton,bound,new_order,&m);
#else
  matrix::Matrix_base<standardrepk::CharCoeff> ktypemat =
    khc.K_type_matrix(singleton,bound,new_order,NULL);
#endif

  std::cout << "Ordering of representations/K-types:\n";
  for (std::vector<standardrepk::seq_no>::const_iterator
	 it=new_order.begin(); it!=new_order.end(); ++it)
    khc.print(std::cout,khc.rep_no(*it)) << ", height " << khc.height(*it)
       << std::endl;

#ifdef VERBOSE
  prettyprint::printMatrix(std::cout<<"Triangular system:\n",m,3);
#endif

  prettyprint::printMatrix(f<<"Matrix of K-type multiplicites:\n",ktypemat,3);

} // |Ktypemat_f|

void qKtypemat_f()
{
  RealReductiveGroup& G = realmode::currentRealGroup();

  standardrepk::qKhatContext khc(G);

  StandardRepK sr=interactive::get_standardrep(khc);

  {
    size_t witness;
    if (not khc.isNormal(sr,witness))
    {
      khc.print(std::cout << "Representation ",sr)
        << " is not normal, as witnessed by coroot sum "
	<< khc.info(sr.Cartan()).coroot_sum(witness)
	<< ".\n";
      return;
    }
    if (not khc.isStandard(sr,witness))
    {
      khc.print(std::cout << "Representation ",sr)
        << " is not standard, as witnessed by coroot "
	<< G.rootDatum().coroot(khc.fiber(sr).simpleImaginary(witness))
	<< ".\n";
      return;
    }
    if (not khc.isFinal(sr,witness))
    {
      khc.print(std::cout << "Representation ",sr)
        << " is not final, as witnessed by coroot "
	<< G.rootDatum().coroot(khc.fiber(sr).simpleReal(witness)) << ".\n";
      return;
    }
  }

  standardrepk::q_combin c=khc.standardize(sr);

  if (c.empty())
  {
    khc.print(std::cout << "Representation ",sr) << " is zero.\n";
    return;
  }

  assert(c.size()==1);
  assert(khc.rep_no(c.begin()->first)==sr);

  khc.print(std::cout << "Height of representation ",sr) << " is "
		      << khc.height(c.begin()->first) << ".\n";
  unsigned long bound=
    interactive::get_bounded_int(interactive::common_input(),
				 "Give height bound: ",
				 9999);

  ioutils::OutputFile f;

  std::set<standardrepk::q_equation> singleton;
  singleton.insert(khc.mu_equation(c.begin()->first,bound));

  {
    standardrepk::q_equation init=*singleton.begin();
    khc.print(f << "Initial formula: mu(",khc.rep_no(init.first))
      << ") =\n";
    {
      std::ostringstream s; khc.print(s,init.second);
      ioutils::foldLine(f,s.str(),"+\n- ","",1) << std::endl;
    }
  }

#ifdef VERBOSE

  std::vector<standardrepk::q_equation> system =
    khc.saturate(singleton,bound);
  std::cout << "System of equations:\n";
  for (size_t i=0; i<system.size(); ++i)
  {
    const standardrepk::q_equation& si=system[i];
    khc.print(std::cout<< si.first << ' ',khc.rep_no(si.first))
      << " [" << khc.height(si.first) << "]\n     ";
    for (standardrepk::q_combin::const_iterator
	   it=si.second.begin(); it!=si.second.end(); ++it)
      std::cout << '+' << it->second << "*I(" << it->first << ')';
    std::cout << std::endl;
  }
#endif

  std::vector<standardrepk::seq_no> new_order;

#ifdef VERBOSE
  matrix::Matrix_base<standardrepk::q_CharCoeff> m;
  matrix::Matrix_base<standardrepk::q_CharCoeff> ktypemat =
    khc.K_type_matrix(singleton,bound,new_order,&m);
#else
  matrix::Matrix_base<standardrepk::q_CharCoeff> ktypemat =
    khc.K_type_matrix(singleton,bound,new_order,NULL);
#endif

  f << "Ordering of representations/K-types:\n";
  for (std::vector<standardrepk::seq_no>::const_iterator
	 it=new_order.begin(); it!=new_order.end(); ++it)
    khc.print(f,khc.rep_no(*it)) << ", height " << khc.height(*it)
				 << std::endl;

#ifdef VERBOSE
  prettyprint::printMatrix(std::cout<<"Triangular system:\n",m,3);
#endif

  prettyprint::printMatrix(f<<"Matrix of K-type multiplicites:\n",ktypemat,3);
} // |qKtypemat_f|

void mod_lattice_f()
{
  RealReductiveGroup& G = realmode::currentRealGroup();

  unsigned long cn=interactive::get_Cartan_class(G.Cartan_set());

  WeightInvolution q = G.cartan(cn).involution();
  for (size_t j = 0; j<q.numRows(); ++j)
    q(j,j) -= 1;

  CoeffList factor;
  int_Matrix b = matreduc::adapted_basis(q,factor);

  RankFlags units, doubles;
  unsigned n1=0,n2=0;

  for (size_t i=0; i<factor.size(); ++i)
    if (factor[i]==1)
      units.set(i),++n1;
    else if (factor[i]==2)
      doubles.set(i),++n2;

   std::cout << "At Cartan class " << cn;
   if (n1+n2==0)
     std::cout << " weights are used unchanged";
   else
   {
     std::cout << " weights are modulo";
     if (n1>0)
     {
       std::cout << " multiples of ";
       for (RankFlags::iterator it=units.begin(); it(); ++it,--n1)
	 std::cout << b.column(*it) << (n1>2 ? ", " : n1>1 ? ", and " : "");
       if (n2>0)
	 std::cout << " and";
     }
     if (n2>0)
     {
       std::cout << " even multiples of ";
       for (RankFlags::iterator it=doubles.begin(); it(); ++it,--n2)
	 std::cout << b.column(*it) << (n2>2 ? ", " : n2>1 ? ", and " : "");
     }
   }
   std::cout << ".\n";

} // |mod_lattice_f|

void branch_f()
{
  RealReductiveGroup& G = realmode::currentRealGroup();

  standardrepk::KhatContext khc(G);

  StandardRepK sr=interactive::get_standardrep(khc);

  {
    size_t witness;
    if (not khc.isStandard(sr,witness))
    {
      khc.print(std::cout << "Representation ",sr)
        << " is not standard, as witnessed by coroot "
	<< G.rootDatum().coroot(khc.fiber(sr).simpleImaginary(witness))
	<< ".\n";
      return;
    }
    if (not khc.isFinal(sr,witness))
    {
      khc.print(std::cout << "Representation ",sr)
        << " is not final, as witnessed by coroot "
	<< G.rootDatum().coroot(khc.fiber(sr).simpleReal(witness)) << ".\n";
      return;
    }
  }

  standardrepk::combination c=khc.standardize(sr);

  if (c.empty())
  {
    khc.print(std::cout << "Representation ",sr) << " is zero.\n";
    return;
  }

  assert(c.size()==1 and khc.rep_no(c.begin()->first)==sr);

  khc.print(std::cout << "Height of representation ",sr) << " is "
    << khc.height(c.begin()->first) << ".\n";
  unsigned long bound=
    interactive::get_bounded_int(interactive::common_input(),
				 "Give height bound: ",
				 9999);

  ioutils::OutputFile f;

  standardrepk::combination result=khc.branch(c.begin()->first,bound);

  {
    std::ostringstream s; khc.print(s,result);
    ioutils::foldLine(f,s.str(),"+","",1) << std::endl;
  }

} // |branch_f|

void qbranch_f()
{
  RealReductiveGroup& G = realmode::currentRealGroup();

  standardrepk::qKhatContext khc(G);

  StandardRepK sr=interactive::get_standardrep(khc);

  {
    size_t witness;
    if (not khc.isStandard(sr,witness))
    {
      khc.print(std::cout << "Representation ",sr)
        << " is not standard, as witnessed by coroot "
	<< G.rootDatum().coroot(khc.fiber(sr).simpleImaginary(witness))
	<< ".\n";
      return;
    }
    if (not khc.isFinal(sr,witness))
    {
      khc.print(std::cout << "Representation ",sr)
        << " is not final, as witnessed by coroot "
	<< G.rootDatum().coroot(khc.fiber(sr).simpleReal(witness)) << ".\n";
      return;
    }
  }

  standardrepk::q_combin c=khc.standardize(sr);

  if (c.empty())
  {
    khc.print(std::cout << "Representation ",sr) << " is zero.\n";
    return;
  }

  assert(c.size()==1 and khc.rep_no(c.begin()->first)==sr);

  khc.print(std::cout << "Height of representation ",sr) << " is "
    << khc.height(c.begin()->first) << ".\n";
  unsigned long bound=
    interactive::get_bounded_int(interactive::common_input(),
				 "Give height bound: ",
				 9999);

  ioutils::OutputFile f;

  standardrepk::q_combin result=khc.branch(c.begin()->first,bound);

  {
    std::ostringstream s; khc.print(s,result);
    ioutils::foldLine(f,s.str(),"+","",1) << std::endl;
  }

} // |qbranch_f|


// Block mode functions



// Empty mode functions


/*
  Function invoked by the "srtest" command.
*/
void srtest_f()
{
  RealReductiveGroup& G = realmode::currentRealGroup();
  const KGB& kgb = G.kgb();

  unsigned long x=interactive::get_bounded_int
    (interactive::sr_input(),"Choose KGB element: ",G.kgb().size());

  prettyprint::printVector(std::cout<<"2rho = ",G.rootDatum().twoRho())
    << std::endl;

  Weight lambda=
    interactive::get_weight(interactive::sr_input(),
			    "Give lambda-rho: ",
			    G.rank());
  standardrepk::KhatContext khc(G);

  StandardRepK sr=khc.std_rep_rho_plus(lambda,kgb.titsElt(x));

  (lambda *= 2) += G.rootDatum().twoRho();
  prettyprint::printVector(std::cout << "Weight (1/2)",lambda);
  prettyprint::printVector(std::cout << " converted to (1/2)",khc.lift(sr));

  const TwistedInvolution& canonical =
    G.complexGroup().twistedInvolution(sr.Cartan());
  if (kgb.involution(x)!=canonical)
    prettyprint::printWeylElt(std::cout << " at involution ",
			      canonical, G.weylGroup());
  std::cout << "\nHeight is " << khc.height(sr) << std::endl;

  khc.go(sr);
}

bool examine(RealReductiveGroup& G)
{
  const WeylGroup& W = G.weylGroup();
  const KGB& kgb=G.kgb();
  size_t l = W.length(kgb.involution(0)),t;
  for (size_t i=1; i<kgb.size(); ++i)
    if ((t=W.length(kgb.involution(i)))<l)
      return false;
    else
      l=t;
  return true;
}

void testrun_f()
{
  unsigned long rank=interactive::get_bounded_int
    (interactive::common_input(),"rank: ",constants::RANK_MAX+1);
  std::cout << "Testing W-length monotonicity.\n";
  for (testrun::LieTypeIterator it(testrun::Semisimple,rank); it(); ++it)
  {
    std::cout<< *it << std::endl;
    size_t count=0;
    for (testrun::CoveringIterator cit(*it); cit(); ++cit)
    {
      if (count>0) std::cout << ',';
      std::cout << ++count;
      PreRootDatum prd = *cit;
      WeightInvolution id(prd.rank()); // identity
      ComplexReductiveGroup G(prd,id);
      for (RealFormNbr rf=0; rf<G.numRealForms(); ++rf)
      {
	RealReductiveGroup G_R(G,rf);
	if (not examine(G_R))
	{
	  lietype::InnerClassType ict;
	  for (size_t i=0; i<it->size(); ++i)
	    ict.push_back('e');
	  lietype::Layout lay(*it,ict);
	  realform_io::Interface itf(G,lay);
	  std::cout << " Failure at real form " << itf.out(rf) << std::endl;
	}
	std::cout << std::flush;
      }
    }
    std::cout << '.' << std::endl;
  }

}

void exam_f()
{
  std::cout << "W-length monotine in KGB? "
            << (examine(realmode::currentRealGroup())
		? "yes" : "no")
	    << std::endl;
}

void X_f()
{
  ComplexReductiveGroup& G=mainmode::currentComplexGroup();
  kgb::global_KGB kgb(G); // build global Tits group, "all" square classes
  ioutils::OutputFile f;
  kgb_io::print_X(f,kgb);
}



TorusElement torus_part
  (const RootDatum& rd,
   const WeightInvolution& theta,
   const RatWeight& lambda, // discrete parameter
   const RatWeight& gamma // infinitesimal char
  )
{
  InvolutionData id(rd,theta);
  Weight cumul(rd.rank(),0);
  arithmetic::Numer_t n=gamma.denominator();
  const Ratvec_Numer_t& v=gamma.numerator();
  const RootNbrSet pos_real = id.real_roots() & rd.posRootSet();
  for (RootNbrSet::iterator it=pos_real.begin(); it(); ++it)
    if (rd.coroot(*it).dot(v) %n !=0) // nonintegral
      cumul+=rd.root(*it);
  // now |cumul| is $2\rho_\Re(G)-2\rho_\Re(G(\gamma))$

  return y_values::exp_pi(gamma-lambda+RatWeight(cumul,2));
}

void embedding_f()
{
  RealReductiveGroup& GR = realmode::currentRealGroup();
  ComplexReductiveGroup& G = GR.complexGroup();
  const RootDatum& rd = G.rootDatum();

  Weight lambda_rho =
    interactive::get_weight(interactive::sr_input(),"Give lambda-rho: ",
			    G.rank());

  RatWeight lambda(lambda_rho *2 + rd.twoRho(),2);

  RatWeight nu=
    interactive::get_ratweight
    (interactive::sr_input(),"rational parameter nu: ",rd.rank());

  const KGB& kgb = GR.kgb();
  unsigned long x=interactive::get_bounded_int
    (interactive::common_input(),"KGB element: ",kgb.size());

  WeightInvolution theta = G.involutionMatrix(kgb.involution(x));

  nu = RatWeight // make |nu| fixed by $-\theta$
    (nu.numerator()- theta*nu.numerator(),2*nu.denominator());

  RatWeight gamma = nu + RatWeight
    (lambda.numerator()+theta*lambda.numerator(),2*lambda.denominator());

  ioutils::OutputFile f;
  f << "Infinitesimal character is " << gamma << std::endl;

  SubSystemWithGroup sub = SubSystemWithGroup::integral(rd,gamma);

  WeylWord ww;
  const tits::SubTitsGroup sub_gTg
    (G,sub,G.involutionMatrix(kgb.involution(x)),ww);

  Permutation pi;

  f << "Subsystem on dual side is ";
  if (sub.rank()==0)
    f << "empty.\n";
  else
  {
    f << "of type " << dynkin::Lie_type(sub.cartanMatrix(),true,false,pi)
      << ", with roots ";
    for (weyl::Generator s=0; s<sub.rank(); ++s)
      f << sub.parent_nr_simple(pi[s]) << (s<sub.rank()-1 ? "," : ".\n");
  }
  f << "Twisted involution in subsystem: " << ww << ".\n";

  weyl::TI_Entry::Pooltype pool;
  HashTable<weyl::TI_Entry,unsigned int> hash_table(pool);
  std::vector<unsigned int> stops;
  {
    std::vector<TwistedInvolution> queue(1,TwistedInvolution());
    size_t qi = 0; // |queue| inspected up to |qi|
    const TwistedWeylGroup& tW = sub_gTg; // base object, subgroup

    while (qi<queue.size())
    {
      weyl::TI_Entry tw=queue[qi++];
      if (hash_table.find(tw)==hash_table.empty) // Cartan class not seen yet
      { stops.push_back(hash_table.size());
	// generate like kgb::FiberData::complete_class|
	for (unsigned int i=hash_table.match(tw); i<hash_table.size(); ++i)
	{
	  tw=pool[i];
	  for (weyl::Generator s=0; s<tW.rank(); ++s)
	    if (tW.hasTwistedCommutation(s,tw)) // |s| real or imaginary
	    {
	      if (not tW.hasDescent(s,tw)) // |s| imaginary
		queue.push_back(tW.prod(s,tw));
	    }
	    else // |s| complex
	      hash_table.match(tW.twistedConjugated(tw,s));
	} // |for(i)|
      } // |if| new Cartan class
    } // |while| queue not completely inspected
  } // forget |queue|
  stops.push_back(~0); // sentinel
  size_t si=0; // index into |stops|
  for (unsigned int i=0; i<pool.size(); ++i)
  {
    if (i==stops[si])
    { (std::ostream&)f << std::endl; ++si; } // separate classes
    TwistedInvolution tw=pool[i];
    f << sub_gTg.base_point_offset(tw).log_2pi()
      << "  \t@  " << sub_gTg.word(tw) <<std::endl;
  }
} // |embedding_f|

void test_f()
{
  // replace this with your definition
} // |test_f|


  // Block mode functions

void hblock_f()
{
  try
  {
    RealReductiveGroup& GR = realmode::currentRealGroup();
    RealReductiveGroup& dual_GR = blockmode::currentDualRealGroup();
    ioutils::OutputFile f;
    const KGB& kgb = GR.kgb();
    const KGB& dual_kgb = dual_GR.kgb();
    blocks::hBlock block = hBlock(kgb, dual_kgb);

    block_io::printHBlockD(f,block);

    Block* block_pointer = new Block
      (Block::build(realmode::currentRealGroup(),
		    blockmode::currentDualRealGroup()));
    kl::KLContext* klc_pointer=new kl::KLContext(*block_pointer);
    klc_pointer->fill();

    size_t hbsize = block.hsize();
    size_t klsize = klc_pointer->size();
    std::vector<bool> flags(klsize,false);
    for (size_t i=0; i<hbsize; i++) {
      BlockElt j = block.hfixed(i);
      flags[j] = true;
    }

    size_t count = 0;
    int width = ioutils::digits(klc_pointer->size()-1,10ul);
    int tab = 2;

    for (size_t y = 0; y < klc_pointer->size(); ++y)
      if (flags[y])
      {
	f << std::setw(width) << y << ": ";
	bool first = true;

	for (size_t x = 0; x <= y; ++x)
	  if (flags[x])
	  {
	    const kl::KLPol& pol = klc_pointer->klPol(x,y);
	    if (pol.isZero())
	      continue;
	    if (first)
	    {
	      f << std::setw(width) << x << ": ";
	      first = false;
	    }
	    else
	    {
	      f << std::setw(width+tab)<< ""
		<< std::setw(width) << x << ": ";
	    }
	    prettyprint::printPol(f,pol,"q");
	    f << "" << std::endl;
	    ++count;
	  }

	f << "" << std::endl;
      }

    /*
      kl::KLContext klc(block);
      klc.fill(z,false);

      typedef Polynomial<int> Poly;
      typedef std::map<BlockElt,Poly> map_type;
      map_type acc;
      unsigned int parity = block.length(z)%2;
      for (size_t x = 0; x <= z; ++x)
      {
      const kl::KLPol& pol = klc.klPol(x,z);
      if (not pol.isZero())
      {
      Poly p(pol); // convert
      if (block.length(x)%2!=parity)
      p*=-1;
      BlockEltList nb=block.nonzeros_below(x);
      for (size_t i=0; i<nb.size(); ++i)
      {
      std::pair<map_type::iterator,bool> trial =
      acc.insert(std::make_pair(nb[i],p));
      if (not trial.second) // failed to create a new entry
      trial.first->second += p;
      } // |for (i)| in |nb|
      } // |if(pol!=0)|
      } // |for (x<=z)|


      f << (block.singular_simple_roots().any() ? "(cumulated) " : "")
      << "KL polynomials (-1)^{l(" << z << ")-l(x)}*P_{x," << z << "}:\n";
      int width = ioutils::digits(z,10ul);
      for (map_type::const_iterator it=acc.begin(); it!=acc.end(); ++it)
      {
      BlockElt x = it->first;
      const Poly& pol = it->second;
      if (not pol.isZero())
      {
      f << std::setw(width) << x << ": ";
      prettyprint::printPol(f,pol,"q") << std::endl;
      }
      }
    */
  }
  catch (error::MemoryOverflow& e)
  {
    e("error: memory overflow");
  }
  catch (error::InputError& e)
  {
    e("aborted");
  }
  catch (std::exception& e)
  {
    std::cerr << "error occurrend: " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << std::endl << "unidentified error occurred" << std::endl;

  }

} // |hblock_f|


//Help commands

void roots_rootbasis_h()
{
  io::printFile(std::cerr,"roots_rootbasis.help",io::MESSAGE_DIR);
}

void coroots_rootbasis_h()
{
  io::printFile(std::cerr,"coroots_rootbasis.help",io::MESSAGE_DIR);
}

void posroots_rootbasis_h()
{
  io::printFile(std::cerr,"posroots_rootbasis.help",io::MESSAGE_DIR);
}

void poscoroots_rootbasis_h()
{
  io::printFile(std::cerr,"poscoroots_rootbasis.help",io::MESSAGE_DIR);
}

void checkbasept_h()
{
  io::printFile(std::cerr,"checkbasept.help",io::MESSAGE_DIR);
}

void sub_KGB_h()
{
  io::printFile(std::cerr,"sub_KGB.help",io::MESSAGE_DIR);
}

void trivial_h()
{
  io::printFile(std::cerr,"trivial.help",io::MESSAGE_DIR);
}

void Ktypemat_h()
{
  io::printFile(std::cerr,"Ktypemat.help",io::MESSAGE_DIR);
}

void Ktypeform_h()
{
  io::printFile(std::cerr,"Ktypeform.help",io::MESSAGE_DIR);
}

void mod_lattice_h()
{
  io::printFile(std::cerr,"mod_lattice.help",io::MESSAGE_DIR);
}

void branch_h()
{
  io::printFile(std::cerr,"branch.help",io::MESSAGE_DIR);
}

void srtest_h()
{
  io::printFile(std::cerr,"srtest.help",io::MESSAGE_DIR);
}

void hkllist_f()
{
  RealReductiveGroup& GR = realmode::currentRealGroup();
  RealReductiveGroup& dual_GR = blockmode::currentDualRealGroup();
  ioutils::OutputFile file;
  std::ostream& strm = file;

  std::cerr << "Building KGB/Dual KGB ... ";
  const KGB& kgb = GR.kgb();
  const KGB& dual_kgb = dual_GR.kgb();
  std::cerr << "Done." << std::endl;

  std::cerr << "Building hblock ... ";
  blocks::hBlock block = hBlock(kgb, dual_kgb);
  std::cerr << "Done." << std::endl;

  std::cerr << "Computing twisted KL polynomials ... ";
  kl::hKLContext* hklc = new kl::hKLContext(block);
  hklc->fill();
  std::cerr << "Done." << std::endl;

  // sort the polynomials before printing
  const kl::hKLStore& plist = hklc->getPolyList();
  std::vector<kl::hKLPol> splist;
  size_t psize = plist.size();
  for (kl::KLIndex i=1; i<psize; i++) {
    splist.push_back(plist[i]);
  }
  strm << "Printing " << psize-1 << " twisted KL polynomials:" << std::endl;

  // sort
  std::sort(splist.begin(),splist.end(),polynomials::compare<kl::hKLCoeff>);

  // write the polynomials
  for (size_t i=0; i<psize-1; i++) {
    prettyprint::printPol(strm,splist[i],"q");
    strm << std::endl;
  }
}

void hklbasis_f()
{
  RealReductiveGroup& GR = realmode::currentRealGroup();
  RealReductiveGroup& dual_GR = blockmode::currentDualRealGroup();
  ioutils::OutputFile file;
  std::ostream& strm = file;

  std::cerr << "Building KGB/Dual KGB ... ";
  const KGB& kgb = GR.kgb();
  const KGB& dual_kgb = dual_GR.kgb();
  std::cerr << "Done." << std::endl;

  std::cerr << "Building hblock ... ";
  blocks::hBlock block = hBlock(kgb, dual_kgb);
  std::cerr << "Done." << std::endl;

  std::cerr << "Computing twisted KL polynomials ... ";
  kl::hKLContext* hklc = new kl::hKLContext(block);
  hklc->fill();
  std::cerr << "Done." << std::endl;

  // print the full basis
  size_t hsize = hklc->getSize();
  int width = ioutils::digits(hsize-1,10ul);
  strm << "Printing " << hsize << " elements with " << hklc->getPolyList().size()-1 << " distinct polynomials:" << std::endl;
  for (size_t y=0; y<hsize; y++) {
    //for (size_t y=hsize; y-->0;) {
    strm << std::setw(width) << y << ": ";
    bool first = true;

    for (size_t x = 0; x <= y; ++x) {
      const kl::hKLPol& pol = hklc->getPoly(x,y);
      if (pol.isZero()) continue;
      if (first) {
	strm << std::setw(width) << x << ": ";
	first = false;
      }
      else {
	strm << std::setw(width+2)<< "" << std::setw(width) << x << ": ";
      }
      prettyprint::printPol(strm,pol,"q");
      strm << std::endl;
    }

    strm << std::endl;
  }
}

void hkltest_f()
{
  RealReductiveGroup& GR = realmode::currentRealGroup();
  RealReductiveGroup& dual_GR = blockmode::currentDualRealGroup();
  ioutils::OutputFile file;
  std::ostream& strm = file;

  std::cerr << "Building KGB/Dual KGB ... ";
  const KGB& kgb = GR.kgb();
  const KGB& dual_kgb = dual_GR.kgb();
  std::cerr << "Done." << std::endl;

  std::cerr << "Building hblock ... ";
  blocks::hBlock *hblock = new hBlock(kgb, dual_kgb);
  std::cerr << "Done." << std::endl;

  std::cerr << "Computing KL polynomials ... ";
  kl::KLContext* klc=new kl::KLContext(*hblock);
  klc->fill();
  std::cerr << "Done." << std::endl;

  std::cerr << "Computing twisted KL polynomials ... ";
  kl::hKLContext* hklc = new kl::hKLContext(*hblock);
  hklc->fill();
  std::cerr << "Done." << std::endl;

  std::cerr << "Checking polynomials ... ";
  // compare the two sets of polynomials and
  // test for obvious errors
  size_t count = 0;
  size_t hbsize = hblock->hsize();
  for (BlockElt y=0; y<hbsize; y++) {
    BlockElt by = hblock->hfixed(y);
    for (BlockElt x=0; x<=y; x++) {
      BlockElt bx = hblock->hfixed(x);
      const kl::KLPol& pol = klc->klPol(bx,by);
      const kl::hKLPol& hpol = hklc->getPoly(x,y);
      if (pol != hpol) {
	size_t d1 = pol.degree();
	size_t d2 = hpol.degree();
	size_t top = (d1 >= d2) ? d1 : d2;

	// check the coefficients
	for (size_t i=0; i<=top; i++) {
	  kl::hKLCoeff c1 = (i<=d1) ? pol[i] : 0;
	  kl::hKLCoeff c2 = (i<=d2) ? hpol[i] : 0;
	  if (c2 < 0) c2 = -c2;
	  if (c2 > c1) {
            std::cerr << std::endl;
	    std::cerr << "WARNING: twisted KL coefficient exceeds regular coefficient in absolute value for P_{" << x << "," << y << "}" << std::endl;
	    std::cerr << "Aborting" << std::endl;
	    return;
	  }
	  if ((c1-c2) % 2) {
            std::cerr << std::endl;
	    std::cerr << "WARNING: twisted KL coefficient not congruent mod 2 for P_{" << x << "," << y << "}" << std::endl;
	    std::cerr << "Aborting" << std::endl;
	    return;
	  }
	}
      }
    }
  }
  std::cerr << "Passed!" << std::endl;

  int tab = 2;
  int width = ioutils::digits(hbsize-1,10ul);
  strm << "Printing " << hbsize << " elements with " << hklc->getPolyList().size()-1 << " distinct polynomials:" << std::endl;
  for (BlockElt y=0; y<hbsize; y++) {
    bool first = true;
    BlockElt by = hblock->hfixed(y);
    strm << std::setw(width) << y << ": ";

    for (BlockElt x=0; x<=y; x++) {
      BlockElt bx = hblock->hfixed(x);
      const kl::KLPol& pol = klc->klPol(bx,by);
      const kl::hKLPol& hpol = hklc->getPoly(x,y);

      if (pol.isZero() && hpol.isZero()) continue;
      if (first) {
	strm << std::setw(width) << x << ": ";
	prettyprint::printPol(strm,pol,"q");
	if (hpol != pol) {
	  strm << std::endl;
	  strm << std::setw(2*width+2*tab) << "";
	  prettyprint::printPol(strm,hpol,"q");
	}
	first = false;
      }
      else {
	strm << std::setw(width+tab) << "" << std::setw(width) << x << ": ";
	prettyprint::printPol(strm,pol,"q");
	if (hpol != pol) {
	  strm << std::endl;
	  strm << std::setw(2*width+2*tab) << "";
	  prettyprint::printPol(strm,hpol,"q");
	}
      }
      strm << "" << std::endl;
      ++count;
    }

    strm << "" << std::endl;
  }

  // free memory
  delete hblock;
  delete klc;
  delete hklc;
}

void nblock_h()
{
  io::printFile(std::cerr,"nblock.help",io::MESSAGE_DIR);
}

} // |namespace|

} // |namespace atlas|

