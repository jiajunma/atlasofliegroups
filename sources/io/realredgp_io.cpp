/*
  This is realredgp_io.cpp

  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Lie Groups and Representations

  For license information see the LICENSE file
*/

#include <iostream>
#include <sstream>
#include <cassert>

#include "realredgp_io.h"

#include "arithmetic.h"	// |remainder|
#include "basic_io.h"	// |seqPrint|
#include "cartan_io.h"// |printcartanClass|
#include "complexredgp.h"
#include "complexredgp_io.h" // use of its |Interface| class
#include "graph.h"	// |OrientedGraph|
#include "ioutils.h"	// |foldLine|
#include "partition.h"
#include "poset.h"
#include "realredgp.h"
#include "realweyl.h"	// |RealWeyl| class
#include "realweyl_io.h" // |printBlockStabilizer|

namespace atlas {


/*****************************************************************************

        Chapter I -- Functions declared in realredgp_io.h

******************************************************************************/

namespace realredgp_io {

std::ostream& printBlockStabilizer(std::ostream& strm,
				   RealReductiveGroup& G_R,
				   size_t cn, RealFormNbr drf)

{
  ComplexReductiveGroup& G_C = G_R.complexGroup();
  const RootDatum& rd = G_R.rootDatum();
  const WeylGroup& W = G_R.weylGroup();

  RealFormNbr rf = G_R.realForm();

  unsigned long x = G_C.representative(rf,cn);
  unsigned long y = G_C.dualRepresentative(drf,cn);
  const CartanClass& cc = G_C.cartan(cn);

  realweyl::RealWeyl rw(cc,x,y,rd,W);
  realweyl::RealWeylGenerators rwg(rw,cc,rd);

  realweyl_io::printBlockStabilizer(strm,rw,rwg);

  // check if the size is correct
  size::Size c;
  realweyl::blockStabilizerSize(c,rw);
  c *= G_C.fiberSize(rf,cn);
  c *= G_C.dualFiberSize(drf,cn);
  c *= cc.orbitSize();
  assert(c == W.order());

  return strm;
}

// Print information about all the Cartan classes for |G_RI.realGroup()|.
std::ostream& printCartanClasses(std::ostream& strm,
				 RealFormNbr rf,
				 complexredgp_io::Interface& G_CI)
{
  const BitMap& b = G_CI.complexGroup().Cartan_set(rf);
  bool first = true;

  for (BitMap::iterator i = b.begin(); i(); ++i)
  {
    if (first)
      first = false;
    else
      strm << std::endl << std::endl;
    strm << "Cartan #" << *i << ":" << std::endl;
    cartan_io::printCartanClass(strm,*i,G_CI);
  }

  return strm;
}

/* the function below is much like |kgb_io::printBruhatOrder|, but cannot
   call that function, which needs a different type, nameley |BruhatOrder|.
 */

// Print the Hasse diagram of the Cartan ordering of G_R.
std::ostream& printCartanOrder(std::ostream& strm,
			       const RealReductiveGroup& G_R)
{
  const poset::Poset& p = G_R.complexGroup().Cartan_ordering();

  // get Hasse diagram of the Cartan classes for |G_R|
  graph::OrientedGraph g = p.hasseDiagram(G_R.mostSplit());

  strm << "0:" << std::endl; // this is the only minimal element

  // all covering relations in |g| are grouped by lower (covered) element
  for (size_t j = 1; j < g.size(); ++j)
  {
    const graph::VertexList& e = g.edgeList(j);
    if (not e.empty()) // suppress other non-covered elements: not for |G_R|
      basic_io::seqPrint(strm << j << ": ",e.begin(),e.end()) << std::endl;
  }

  return strm;
}


/*
  Print the real Weyl group corresponding to Cartan #cn.

  Precondition: cartan #cn is defined for this real form.
*/
std::ostream& printRealWeyl(std::ostream& strm,
			    RealReductiveGroup& G_R,
			    size_t cn)
{
  ComplexReductiveGroup& G_C = G_R.complexGroup();

  RealFormNbr rf = G_R.realForm();

  const RootDatum& rd = G_C.rootDatum();
  const WeylGroup& W = G_C.weylGroup();
  const CartanClass& cc = G_C.cartan(cn);
  cartanclass::AdjointFiberElt x = G_C.representative(rf,cn);

  realweyl::RealWeyl rw(cc,x,0,rd,W);
  realweyl::RealWeylGenerators rwg(rw,cc,rd);

  realweyl_io::printRealWeyl(strm,rw,rwg);

  // check if the size is correct
  size::Size c;
  realweyl::realWeylSize(c,rw);
  c *= G_C.fiberSize(rf,cn);
  c *= cc.orbitSize();
  assert(c == W.order());

  return strm;
}

/*
  Synopsis: outputs information about the strong real forms of G.

  Explanation: the inverse image in \X of a class of weak real forms of G is
  of the form Z.\X(z), where z is an admissible value for x^2 for a strong real
  form of that class; so there is associated to the class of weak real forms
  a coset (1+\delta)(Z).z in Z^\delta. All the various \X(z) for all possible
  choices of z are isomorphic by Z-translation. Therefore it is enough to
  describe the combinatorial structure of one of them, for each class of weak
  real forms. This is a finite problem in all cases.

  We output the orbits of W_im in X(z), which correspond to the various strong
  real forms; we label them with the corresponding weak real form.
*/
std::ostream& printStrongReal(std::ostream& strm,
			      ComplexReductiveGroup& G_C,
			      const realform_io::Interface& rfi,
			      size_t cn)
{
  Fiber fund = G_C.fundamental();
  const CartanClass& cc = G_C.cartan(cn);
  const RealFormNbrList& rfl = G_C.realFormLabels(cn);

  size_t n = cc.numRealFormClasses();

  if (n>1)
    strm << "there are " << n << " real form classes:\n" << std::endl;

  for (size_t csc=0; csc<n; ++csc)
  {
    // print information about the square of real forms, in center
    {
      RealFormNbr wrf=cc.fiber().realFormPartition().classRep(csc);
      RealFormNbr fund_wrf= rfl[wrf]; // lift weak real form to |fund|
      cartanclass::square_class f_csc=fund.central_square_class(fund_wrf);
      // having the square class number of the fundamental fiber, get grading
      Grading base_grading =
	tits::square_class_grading_offset(fund,f_csc,G_C.rootSystem());

      RatWeight z (G_C.rank());
      for (Grading::iterator it=base_grading.begin(); it(); ++it)
	z += G_C.rootDatum().fundamental_coweight(*it);

      Ratvec_Numer_t& zn = z.numerator();
      for (size_t i=0; i<z.size(); ++i)
        zn[i]=arithmetic::remainder(zn[i],z.denominator());
      strm << "class #" << f_csc
	   << ", possible square: exp(2i\\pi(" << z << "))" << std::endl;
    }

    const Partition& pi = cc.fiber_partition(csc);

    unsigned long c = 0;

    for (Partition::iterator i(pi); i(); ++i,++c)
    {
      std::ostringstream os;
      RealFormNbr rf = rfl[cc.toWeakReal(c,csc)];
      os << "real form #" << rfi.out(rf) << ": ";
      basic_io::seqPrint(os,i->first,i->second,",","[","]")
	<< " (" << i->second - i->first << ")" << std::endl;
      ioutils::foldLine(strm,os.str(),"",",");
    }

    if (n>1)
      strm << std::endl;
  }

  return strm;
}

} // |namespace realredgp_io|

} // |namespace atlas|
