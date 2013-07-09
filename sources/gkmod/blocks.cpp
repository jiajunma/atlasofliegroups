/*
  This is blocks.cpp

  Copyright (C) 2004,2005 Fokko du Cloux
  Copyright (C) 2007--2013 Marc van Leeuwen
  Part of the Atlas of Lie Groups and Representations

  For license information see the LICENSE file
*/

#include "blocks.h"

#include <cassert>
#include <vector>
#include <deque>
#include <set> // for |insertAscents|
#include <algorithm>
#include <iterator>

#include "arithmetic.h"

#include "tags.h"
#include "hashtable.h"

#include "bruhat.h"	// construction
#include "complexredgp.h"
#include "realredgp.h"
#include "subsystem.h"
#include "y_values.h"
#include "kgb.h"
#include "weyl.h"
#include "kl.h"		// destruction
#include "repr.h" // use of |StandardRepr| in |non_integral_block| constructor

/*
  Our task in the traditional setup (the constructor for |Block|) is fairly
  simple: given the one sided parameter sets (|KGB|) for the real form and for
  the dual real form, which are fibred over the sets of twisted involutions
  for the Weyl group and dual Weyl group respectively, we must form the fibred
  product over corresponding pairs of twisted involutions, and equip the
  resulting structure with relations inherited from the kgb structures.

  One point to be resolved here is the relation between a twisted involution
  and the corresponding dual twisted involution; it is implemented in the
  function |dual_involution| below. On the level of involution matrices acting
  on the character and cocharacter lattices, the relation is minus transpose;
  this has to be translated into a relation between Weyl group elements.

  Another important point here is matching the local structure of the two KGB
  sets to define the local relations in the block structure. The combinations
  possible of local structures in the KGB sets, as given by |gradings::Status|
  values, are limited by the relationship between involution and dual
  involution: a generator that is complex for one is so as well for the other,
  while the remaining generators are imaginary for one are real for the other.

  It turns out that possibilites are even more restricted then implied by the
  above relation, and every block element $z=(x,y)$ occurs, for each generator
  |s|, in one of the following five configurations. At the left we depict the
  coordinate |x| in the kgb set, to the right of it the coordinte |y| in the
  dual kgb set; in the former case going down increases the length, in the
  latter it decreases the length. At the right we describe the local block
  structure; the terminology used there refers mostly to what happened for |x|,
  and in particular going down is considered to be an ascent in the block.

  If |s| is complex for the involution and its dual, one has cross actions

             ( x      ,     y   )    ComplexAscent       z
	       |            |                            |
	       |            |                            |
	     ( x'     ,     y'  )    ComplexDescent      z'

  If |s| is imaginary noncompact for the involution, it will be real, and in
  the image of the Cayley transform, for the dual involution. Either the
  Cayley image of the |x| coordinate is shared with that of another KGB
  element and the |y| coordinate has a single-valued inverse Cayley transform
  (type I situation), or the Cayley image of the |x| coordinate is unshared,
  and the |y| coordinate has a double-valued inverse Cayley transform.

      ( x      x'    , s^y   )    Imaginary Type I (twice)    z     z'
	 \    /         |                                      \   /
	  \  /          |                                       \ /
      (  s^x=s^x'    ,  y    )         Real Type I              s^z



      (    x    ,   s^y=s^y' )       Imaginary Type II           z
           |          / \                                       / \
	   |         /   \                                     /   \
      (   s^x   ,   y     y' )     Real Type II (twice)     s^z_1  s^z_2

  If |s| is imaginary compact for the involution, it will be real and not in
  the image of the Cayley transform for the dual involution. No Cayley
  transorm will be defined for the |x| coordinate, and no inverse Cayley
  transform for the |y| coordinate, and both are fixed by the cross action;
  the situation is called ImaginaryCompact.

      (    x    ,     y     )       ImaginaryCompact             z

  Finally that situation with x and y interchanged is called RealNonparity

      (    x    ,     y     )         RealNonparity              z

  Although the last two cases have no cross action links for |s| to other
  block elements, nor any Cayley or inverse Cayley links for |s|, we consider
  (more in particular |DescentStatus::isDescent| considers) |s| to
  be in the descent set in the ImaginaryCompact case, and not in the descent
  set for the RealNonparity case (note that this is opposite to the status of
  imaginary and real generators in the other (parity) cases). These cases do
  not count as strict descent/ascent however, as is indicated in the
  predicate emthods |isStrictDescent| and |isStrictAscent| below.
*/

namespace atlas {

namespace blocks {




namespace {
  // some auxiliary functions used by methods, but defined near end of file

  // compute descent status of block element, based on its $(x,y)$ parts
DescentStatus descents(KGBElt x, KGBElt y,
		       const KGB_base& kgb, const KGB_base& dual_kgb);

  // compute Hasse diagram of the Bruhat order of a block
std::vector<set::EltList> makeHasse(const Block_base&);


} // namespace

} // namespace blocks

/*****************************************************************************

        Chapter I -- The Block_base class

******************************************************************************/

namespace blocks {

// an auxiliary function:
// we often need to fill the first empty slot of a |BlockEltPair|
inline BlockElt& first_free_slot(BlockEltPair& p)
{
  if (p.first==UndefBlock)
    return p.first;
  else
  {
    assert(p.second==UndefBlock); // there should be an empty slot left
    return p.second;
  }
}

Block_base::Block_base(const KGB& kgb,const KGB& dual_kgb)
  : info(), data(kgb.rank())
  , d_first_z_of_x() // filled below
  , d_bruhat(NULL)
  , klc_ptr(NULL)
{
} // |Block_base::Block_base|

// an almost trivial constructor used for derived non-integral block types
Block_base::Block_base(unsigned int rank)
  : info(), data(rank)
  , d_first_z_of_x()
  , d_bruhat(NULL)
  , klc_ptr(NULL)
{}

Block_base::Block_base(const Block_base& b) // copy constructor, unused
  : info(b.info), data(b.data)
  , d_first_z_of_x(b.d_first_z_of_x)
  , d_bruhat(NULL) // don't care to copy; is empty in |Block::build| anyway
  , klc_ptr(NULL)  // likewise
{
#ifdef VERBOSE // then show that we're called (does not actually happen)
  std::cerr << "copying a block" << std::endl;
#endif
}

Block_base::~Block_base() { delete d_bruhat; delete klc_ptr; }

/*!\brief Look up element by |x|, |y| coordinates

  Precondition: |x| and |y| should be compatible: such a block element exists

  This uses the |d_first_z_of_x| table to locate the range where the |x|
  coordinates are correct; then comparing the given |y| value with the first
  one present for |x| (there must be at least one) we can predict the value
  directly, since for each fixed |x| value the values of |y| are consecutive.
*/
BlockElt Block_base::element(KGBElt xx,KGBElt yy) const
{
  BlockElt first=d_first_z_of_x[xx];
  BlockElt z = first +(yy-y(first));
  assert(z<size() and x(z)==xx and y(z)==yy); // element should be found
  return z;
}

BlockElt Block_base::length_first(size_t l) const
{ // if |length| were an array, we would call |std::lower_bound(begin,end,l)|
  BlockElt min=0, max=size(); // the requested index remains in [min,max]
  while (max>min) // body strictly reduces |max-min| in all cases
  {
    BlockElt z=(min+max)/2;
    if (length(z)>=l)
      max=z; // preserves invariant |length(z)>=l| for all |l>=max|
    else
      min=z+1; // preserves invariant |length(z)<l| for all |l<min|
  }
  assert(min==max);
  return min;
}


/*!
  \brief Tells if s is a strict ascent generator for z.

  Explanation: this means that descentValue(s,z) is one of ComplexAscent,
  ImaginaryTypeI or ImaginaryTypeII.
*/
bool Block_base::isStrictAscent(size_t s, BlockElt z) const
{
  DescentStatus::Value v = descentValue(s,z);
  return not DescentStatus::isDescent(v)
    and v!=DescentStatus::RealNonparity;
}

/*!
  \brief Tells if s is a strict descent generator for z.

  Explanation: this means that descentValue(s,z) is one of ComplexDescent,
  RealTypeI or RealTypeII.
*/
bool Block_base::isStrictDescent(size_t s, BlockElt z) const
{
  DescentStatus::Value v = descentValue(s,z);
  return DescentStatus::isDescent(v)
    and v!=DescentStatus::ImaginaryCompact;
}

/*!
  \brief Returns the first descent for z (the number of a simple root) that is
not imaginary compact, or rank() if there is no such descent.
*/
size_t Block_base::firstStrictDescent(BlockElt z) const
{
  for (size_t s = 0; s < rank(); ++s)
    if (isStrictDescent(s,z))
      return s;

  return rank(); // signal nothing was found
}

/*!
  \brief Returns the first descent for z (the number of a simple root) that is
either complex or real type I; if there is no such descent returns |rank()|
*/
size_t Block_base::firstStrictGoodDescent(BlockElt z) const
{
  for (size_t s = 0; s < rank(); ++s)
    if (isStrictDescent(s,z) and
	descentValue(s,z)!=DescentStatus::RealTypeII)
      return s;

  return rank(); // signal nothing was found
}


KGBElt Block_base::renumber_x(const std::vector<KGBElt>& new_x)
{
  KGBElt x_lim=0; // high-water mark for |new_x| values
  for (BlockElt z=0; z<size(); ++z)
  {
    KGBElt xx=new_x[x(z)];
    if (xx>=x_lim)
      x_lim=xx+1;
    info[z].x = xx;
  }

  KGBEltList xs(info.size());
  for (size_t i=0; i<info.size(); ++i)
    xs[i]=info[i].x;

  Permutation pi_inv = // assigns |z| to its new place
    permutations::standardization(xs,x_lim,&d_first_z_of_x);

  Permutation pi(pi_inv,-1); // assigns to new place its original one

  pi.pull_back(info).swap(info);

  for (weyl::Generator s=0; s<rank(); ++s)
  {
    std::vector<block_fields>& tab_s = data[s];
    pi.pull_back(tab_s).swap(tab_s); // permute fields of |data[s]|
    for (BlockElt z=0; z<size(); ++z)
    {
      tab_s[z].cross_image=pi_inv[tab_s[z].cross_image];
      BlockEltPair& p=tab_s[z].Cayley_image;
      if (p.first!=UndefBlock)
      {
	p.first=pi_inv[p.first];
	if (p.second!=UndefBlock)
	  p.second=pi_inv[p.second];
      }
    }
  }

  return x_lim;
} // |Block_base::renumber_x|

// Here is one method not related to block construction
/*
  The functor \f$T_{\alpha,\beta}\f$

  Precondition: alpha and beta are adjacent roots, of which alpha is a (weak)
  descent for y, while beta is not a descent for y.

  In fact if this is not satisfied, we return a pair of UndefBlock elements
*/

BlockEltPair Block_base::link
  (weyl::Generator alpha,weyl::Generator beta,BlockElt y) const
{
  const DescentStatus& desc=descent(y);

  BlockElt result[2]; // written using iterator
  BlockElt* it = &result[0];

  BlockEltPair p=inverseCayley(alpha,y); // used only in real parity case
  switch (desc[alpha])
  {
  case DescentStatus::ComplexDescent:
    {
      BlockElt y1=cross(alpha,y);
      if (isWeakDescent(beta,y1))
	*it++=y1;
      break;
    }
  case DescentStatus::RealTypeI:
    if (isWeakDescent(beta,p.second))
      *it++=p.second;
    // FALL THROUGH
  case DescentStatus::RealTypeII:
    if (isWeakDescent(beta,p.first))
      *it++=p.first;
    break;
  default: {}
  } // switch(desc[alpha])

  p=cayley(beta,y);
  switch (desc[beta])
  {
  case DescentStatus::ComplexAscent:
    {
      BlockElt y1=cross(beta,y);
      if (not isWeakDescent(alpha,y1))
	*it++=y1;
      break;
    }
  case DescentStatus::ImaginaryTypeII:
    if (not isWeakDescent(alpha,p.second))
      *it++=p.second;
    // FALL THROUGH
  case DescentStatus::ImaginaryTypeI:
    if (not isWeakDescent(alpha,p.first))
      *it++=p.first;
    break;
  default: {}
  } // switch(desc[beta])

  assert(it<=&result[2]);
  while (it<&result[2]) *it++=UndefBlock;

  return std::make_pair(result[0],result[1]);
}

// manipulators

void Block_base::compute_first_zs() // assumes |x| values weakly increase
{
  d_first_z_of_x.resize(xsize()+1);
  KGBElt xx=0;
  d_first_z_of_x[xx]=0; // |d_first_z_of_x[xx]| is smallest |z] with |x(z)>=xx|
  for (BlockElt z=0; z<size(); ++z)
    while (xx<x(z)) // no increment in test: often there should be none at all
      d_first_z_of_x[++xx]=z;

  // now |xx==x(size()-1)|; finish off with a sentinel value |size()|
  do // although the largest |x| should be present: |x(size()-1)==xsize()-1|
    d_first_z_of_x[++xx]=size(); // we don't not depend on that, and fill out
  while (xx<xsize()); // stop after setting |d_first_z_of_x[xsize()]=size()|
}

/*!
  \brief Constructs the BruhatOrder.
  It could run out of memory, but Commit-or-rollback is guaranteed.
*/
void Block_base::fillBruhat()
{
  if (d_bruhat==NULL) // do this only the first time
  {
    std::vector<set::EltList> hd = makeHasse(*this);
    d_bruhat = new BruhatOrder(hd); // commit iff new completed without throwing
  }
}

// computes and stores the KL polynomials
void Block_base::fill_klc(BlockElt last_y,bool verbose)
{
  if (klc_ptr==NULL) // do this only the first time
    klc_ptr=new kl::KLContext(*this);

  klc_ptr->fill(last_y,verbose); // extend tables to contain |last_y|
}


/*****************************************************************************

        Chapter II -- Derived classes of the Block_base class

******************************************************************************/



/*****				Block					****/


Block::Block(const Block& b) // obligatory but in practice unused contruction
  : Block_base(b) // copy
  , tW(b.tW) // share
  , d_Cartan(b.d_Cartan)
  , d_involution(b.d_involution)
  , d_involutionSupport(b.d_involutionSupport)
{}

// Complete the |Block_base| construction, setting |Block|-specific fields
// The real work is done by |Block_base|, |kgb| methods, and |compute_supports|
Block::Block(const KGB& kgb,const KGB& dual_kgb)
  : Block_base(kgb,dual_kgb)
  , tW(kgb.twistedWeylGroup())
  , xrange(kgb.size()), yrange(dual_kgb.size())
  , d_Cartan(), d_involution(), d_involutionSupport() // filled below
{
  const TwistedWeylGroup& dual_W =dual_kgb.twistedWeylGroup();

  std::vector<TwistedInvolution> dual_w; // tabulate bijection |W| -> |dual_W|
  dual_w.reserve(kgb.nr_involutions());
  size_t size=0;
  for (unsigned int i=0; i<kgb.nr_involutions(); ++i)
  {
    const TwistedInvolution w = kgb.nth_involution(i);
    dual_w.push_back(dual_involution(w,tW,dual_W));
    size += kgb.packet_size(w)*dual_kgb.packet_size(dual_w.back());
  }

  info.reserve(size);

  // fill |info|
  for (unsigned int i=0; i<kgb.nr_involutions(); ++i)
  {
    // here is where the fibred product via |dual_w| is built
    const TwistedInvolution w = kgb.nth_involution(i);
    const KGBEltPair x_step = kgb.tauPacket(w);
    const KGBEltPair y_step = dual_kgb.tauPacket(dual_w[i]);

    for (KGBElt x=x_step.first; x<x_step.second; ++x)
      for (KGBElt y=y_step.first; y<y_step.second; ++y)
	info.push_back(EltInfo(x,y,descents(x,y,kgb,dual_kgb),kgb.length(x)));
  } // |for (i)|
  compute_first_zs();

  assert(this->size()==size); // check that |info| has exactly |size| elements

  // Now |element| can be safely called; install cross and Cayley tables

  for (weyl::Generator s = 0; s<rank(); ++s)
  { // the generation below is completely independent for each |s|
    data[s].resize(size);
    for (BlockElt z=0; z<size; ++z)
    {
      data[s][z].cross_image
	= element(kgb.cross(s,x(z)),dual_kgb.cross(s,y(z)));
      switch (descentValue(s,z))
      {
      default: break; // most cases leave |data[s][z].Cayley_image| undefined
      case DescentStatus::ImaginaryTypeII:
	{
	  BlockElt z1=element(kgb.cayley(s,x(z)),
			      dual_kgb.inverseCayley(s,y(z)).second);
	  data[s][z].Cayley_image.second = z1; // double-valued direct Cayley
	  data[s][z1].Cayley_image.first = z; // single-valued inverse Cayley
	}
	// FALL THROUGH
      case DescentStatus::ImaginaryTypeI:
	{
	  BlockElt z0=element(kgb.cayley(s,x(z)),
			      dual_kgb.inverseCayley(s,y(z)).first);
	  data[s][z].Cayley_image.first = z0;
	  // in TypeI, |data[s][z].Cayley_image.second| remains |UndefBlock|
	  first_free_slot(data[s][z0].Cayley_image) = z;
	}
      } // switch
    } // |for (z)|
  } // |for(s)|

  // Complete |Block_base| initialisation by installing the dual links.
  {
    const WeylGroup& Wy = dual_W.weylGroup(); // computation is in this group
    WeylWord w0 = Wy.word(Wy.longest()); // twist y AND cross act by w0
    for (BlockElt z=0; z<size; ++z)
    {
      KGBElt yd= dual_kgb.Hermitian_dual(y(z));
      info[z].dual = element(kgb.Hermitian_dual(x(z)),dual_kgb.cross(w0,yd));
    }
  }

  // Continue filling the fields of the |Block| derived class proper
  d_Cartan.reserve(size);
  d_involution.reserve(size);
  for (BlockElt z=0; z<size; ++z)
  {
    KGBElt xx=x(z);
    d_Cartan.push_back(kgb.Cartan_class(xx));
    d_involution.push_back(kgb.involution(xx));
  }

  compute_supports();
} // |Block::Block(kgb,dual_kgb)|

// Construction function for the |Block| class.
// It is a pseudo constructor method that ends calling main contructor
Block Block::build(ComplexReductiveGroup& G, RealFormNbr rf, RealFormNbr drf)
{
  RealReductiveGroup G_R(G,rf);
  ComplexReductiveGroup dG(G,tags::DualTag()); // the dual group
  RealReductiveGroup dG_R(dG,drf);

  KGB kgb     (G_R, common_Cartans(G_R,dG_R));
  KGB dual_kgb(dG_R,common_Cartans(dG_R,G_R));
  return Block(kgb,dual_kgb); // |kgb| and |dual_kgb| disappear afterwards!
}

// Given both real group and dual real group, we can just call main contructor
Block Block::build(RealReductiveGroup& G_R, RealReductiveGroup& dG_R)
{ return Block(G_R.kgb(),dG_R.kgb()); }

// compute the supports in $S$ of twisted involutions
void Block::compute_supports()
{
  d_involutionSupport.reserve(size()); // its eventual size

  // first compute minimal length cases, probably all for the same involution
  for (BlockElt z=0; z<size() and length(z)==length(0); ++z)
  {
    if (z==0 or involution(z)!=involution(z-1))
    { // compute involution support directly from definition
      RankFlags support;
      WeylWord ww=tW.weylGroup().word(involution(z));
      for (size_t j=0; j<ww.size(); ++j)
	support.set(ww[j]);
      d_involutionSupport.push_back(support);
    }
    else // unchanged involution
      d_involutionSupport.push_back(d_involutionSupport.back()); // duplicate
  }

  // complete involution supports at non-minimal lengths, using previous
  for (BlockElt z=d_involutionSupport.size(); z<size(); ++z)
  {
    size_t s = firstStrictDescent(z);
    assert (s<rank()); // must find one, as we are no longer at minimal length
    DescentStatus::Value v = descentValue(s,z);
    if (v == DescentStatus::ComplexDescent) // cross link
    { // use value from shorter cross neighbour, setting |s| and |twist(s)|
      d_involutionSupport[z] = d_involutionSupport[cross(s,z)];
      d_involutionSupport[z].set(s);
      d_involutionSupport[z].set(tW.twisted(s));
    }
    else // Real Type I or II
    { // use (some) inverse Cayley transform and set |s|
      d_involutionSupport[z] = d_involutionSupport[inverseCayley(s,z).first];
      d_involutionSupport[z].set(s);
    }
  } // |for(z)|
} // |Block::compute_supports|

// 		****	     Nothing else for |Block|		****






param_block::param_block(const Rep_context& rc0, unsigned int rank)
  : Block_base(rank)
  , rc(rc0)
  , infin_char(0) // don't set yet
  , singular() // idem
  , kgb_nr_of()
  , x_of(rc.kgb().size(),UndefKGB)
  , y_pool()
  , y_hash(y_pool)
{}

const TwistedInvolution& param_block::involution(BlockElt z) const
{ return rc.kgb().involution(kgb_nr_of[x(z)]); }

RatWeight param_block::nu(BlockElt z) const
{
  InvolutionNbr i_x = rc.kgb().inv_nr(parent_x(z));
  const WeightInvolution& theta = involution_table().matrix(i_x);
  return RatWeight (gamma().numerator()-theta*gamma().numerator()
		    ,2*gamma().denominator()).normalize();
}

// reconstruct $\lambda-\rho$ from $\gamma$ and the torus part $t$ of $y$
// using $\lambda = \gamma - {1-\theta\over2}.\log{{t\over\pi\ii})$
// the projection factor $1-\theta\over2$ kills the modded-out-by part of $t$
Weight param_block::lambda_rho(BlockElt z) const
{
  RatWeight t =  y_rep(y(z)).log_pi(false);
  InvolutionNbr i_x = rc.kgb().inv_nr(parent_x(z));
  involution_table().real_unique(i_x,t);

  RatWeight lr =
    (infin_char - t - RatWeight(realGroup().rootDatum().twoRho(),2))
    .normalize();
  assert(lr.denominator()==1);
  return Weight(lr.numerator().begin(),lr.numerator().end());
}

// reconstruct $\lambda$ from $\gamma$ and the torus part $t$ of $y$ using the
// formula $\lambda = \gamma - {1-\theta\over2}.\log{{t\over\pi\ii})$
// the projection factor $1-\theta\over2$ kills the modded-out-by part of $t$
RatWeight param_block::lambda(BlockElt z) const
{
  InvolutionNbr i_x = rc.kgb().inv_nr(parent_x(z));
  const WeightInvolution& theta = involution_table().matrix(i_x);
  RatWeight t =  y_rep(y(z)).log_2pi();
  const Ratvec_Numer_t& num = t.numerator();
  return infin_char - RatWeight(num-theta*num,t.denominator());
}


// translation functor from regular to singular $\gamma$ might kill $J_{reg}$
// this depends on the simple coroots for the integral system that vanish on
// the infinitesimal character $\gamma$, namely they make the element zero if
// they define a complex descent, an imaginary compact or a real parity root
bool param_block::survives(BlockElt z) const
{
  const DescentStatus& desc=descent(z);
  for (RankFlags::iterator it=singular.begin(); it(); ++it)
    if (DescentStatus::isDescent(desc[*it]))
      return false;
  return true; // there are no singular simple coroots that are descents
}

// descend through singular simple coroots and return any survivors that were
// reached; they express singular $I(z)$ as sum of 0 or more surviving $I(z')$
BlockEltList param_block::survivors_below(BlockElt z) const
{
  BlockEltList result;
  RankFlags::iterator it;
  do
  {
    const descents::DescentStatus& desc=descent(z);
    for (it=singular.begin(); it(); ++it)
      if (DescentStatus::isDescent(desc[*it]))
      {
	switch (desc[*it])
	{
	case DescentStatus::ImaginaryCompact:
	  return result; // 0
	case DescentStatus::ComplexDescent: z = cross(*it,z);
	  break; // follow descent, no branching
	case DescentStatus::RealTypeII:
	  z=inverseCayley(*it,z).first; break; // follow descent, no branching
	case descents::DescentStatus::RealTypeI:
	  {
	    BlockEltPair iC=inverseCayley(*it,z);
	    BlockEltList left=survivors_below(iC.first);
	    if (result.empty())
	      left.swap(result); // take left result as current value
	    else
	      std::copy(left.begin(),left.end(),back_inserter(result));
	    z = iC.second; // continue with right branch, adding its results
	  }
	  break;
       	default: assert(false); // should never happen, but compiler wants it
	}
	break; // restart outer loop if a descent was applied
      } // |if(descent(*it,z)|
  }
  while (it());
  result.push_back(z);
  return result;
} // |param_block::survivors_below|

void param_block::compute_duals(const ComplexReductiveGroup& G)
{
  const WeightInvolution& delta = G.distinguished();
  const InvolutionTable& i_tab = G.involution_table();

  if (delta*infin_char==infin_char) // for stable blocks compute |delta|-action
    for (BlockElt z=0; z<size(); ++z)
    {
      KGBElt parent_x = kgb_nr_of[x(z)];
      KGBElt dual_x = rc.kgb().Hermitian_dual(parent_x);
      assert(y_hash[y(z)].nr==rc.kgb().inv_nr(parent_x)); // check coherence
      TorusElement t = y_hash[y(z)].t_rep;
      t = y_values::exp_pi(delta*t.log_pi(false)); // twist |t| by |delta|
      KGBElt dual_y = y_hash.find(i_tab.pack(t,rc.kgb().inv_nr(dual_x)));
      if (dual_y!=y_hash.empty)
	info[z].dual = element(x_of[dual_x],dual_y);
    }
}



/*****				gamma_block				****/

gamma_block::gamma_block(const repr::Rep_context& rc,
			 const SubSystemWithGroup& sub,
			 const StandardRepr& sr,
			 BlockElt& entry_element) // output parameter
  : param_block(rc,sub.rank())
{
  infin_char = sr.gamma(); // this field of |param_block| must be set explicitly
  size_t our_rank = sub.rank(); // this is independent of ranks in |GR|
  WeylWord dual_involution; // set in |GlobalTitsGroup| constructor:

  RealReductiveGroup& GR = rc.realGroup();
  const RootDatum& rd = GR.rootDatum();
  const ComplexReductiveGroup& G = GR.complexGroup();
  const Cartan_orbits& i_tab = G.involution_table();
  const KGB& kgb = rc.kgb();

  for (weyl::Generator s=0; s<our_rank; ++s)
    singular.set(s,rd.coroot(sub.parent_nr_simple(s))
		            .dot(infin_char.numerator())==0);

  const RatWeight lambda = rc.lambda(sr);
  KGBElt x=sr.x();

  // first construct global Tits group for |y|s, and |dual_involution|
  const WeightInvolution& theta = kgb.involution_matrix(x);
  const GlobalTitsGroup Tg (sub, theta, dual_involution);

  const TwistedInvolution tw = Tg.weylGroup().element(dual_involution);
  // now |tw| describes |-theta^tr| as twisted involution for |sub|

  // step 1: get a valid value for |y|.
  // Has $t=\exp(\pi\ii(\infin_char-\lambda))$
  TorusElement t = y_values::exp_pi(infin_char-lambda);

  {// step 1.5: correct the grading on the dual imaginary roots.
    assert(Tg.is_valid(GlobalTitsElement(t,tw)));

    Weight tworho_nonintegral_real(GR.rank(),0);
    arithmetic::Numer_t n=infin_char.denominator();
    Ratvec_Numer_t v=infin_char.numerator();
    size_t numpos = rd.numPosRoots();

    for(size_t j=0; j<numpos; ++j)
    {
      RootNbr alpha = rd.posRootNbr(j); // that's |j+numpos|
      if (theta*rd.root(alpha) == -rd.root(alpha) and
	  rd.coroot(alpha).dot(v) %n !=0 ) // whether coroot is NONintegral real
	tworho_nonintegral_real += rd.root(alpha); //if so add it
    }

    RatWeight newcorr(tworho_nonintegral_real,4);
    t +=  y_values::exp_2pi(newcorr); // now the grading on real roots is right

  }
  // save values for |entry_element|
  const KGBElt x_org = x;
  const GlobalTitsElement y_org = GlobalTitsElement(t,tw);
  assert(Tg.is_valid(y_org));

  // step 2: move to the minimal fiber

  { // modify |x| and |y|, descending to minimal element for |subsys|
    weyl::Generator s;
    do
    {
      for(s=0; s<our_rank; ++s)
      {
	KGBElt xx=kgb.cross(sub.to_simple(s),x);
	if (kgb.isDescent(sub.simple(s),xx))
	{
	  if (kgb.status(sub.simple(s),xx)==gradings::Status::Complex)
	  {
	    x = kgb.cross(kgb.cross(sub.simple(s),xx),sub.to_simple(s));
	    // Tg.cross_act(s,y);
	    Tg.complex_cross_act(s,t);
	    break;
	  }
	  else // imaginary
	    if (not Tg.compact(s,t))
	    {
	      xx = kgb.inverseCayley(sub.simple(s),xx).first; // choose one
	      x = kgb.cross(xx,sub.to_simple(s));
	      // Tg.Cayley(s,y);
	      // no need to modify |t| here: forward Cayley for |y|
	      break;
	    }
	} // |if(isDescent)|
      } // |for(s)|
    } while(s<our_rank); // loop until no descents found in |subsys|

    // one might reduce the torus part of |y| here
  } // end of step 2

  InvolutionNbr inv = kgb.inv_nr(x);

  y_hash.match(i_tab.pack(t,inv));

  // step 3: generate imaginary fiber-orbit of |x|'s (|y|'s are unaffected)
  {
    // generating reflections are for imaginary roots for |x|
    RootNbrList gen_root =
      rd.simpleBasis(sub.positive_roots() & i_tab.imaginary_roots(inv));

    KGBEltPair p = kgb.packet(x); // get range of |kgb| that involves |x|
    BitMap seen(p.second-p.first);
    BitMap news = seen;
    news.insert(x-p.first);
    while (news.andnot(seen)) // condition modifies |news| as side effect
    {
      unsigned i=news.front();
      seen.insert(i); // so |i| will be removed from |news| at end of loop
      KGBElt xx=i+p.first;
      for (size_t k=0; k<gen_root.size(); ++k)
	news.insert(kgb.cross(rd.reflectionWord(gen_root[k]),xx)-p.first);
    }

    // now insert elements from |seen| as first $x$-fiber of block
    size_t fs = seen.size(); // this is lower bound for final size, so reserve
    info.reserve(fs);
    for (weyl::Generator s=0; s<our_rank; ++s)
      data[s].reserve(fs);

    for (BitMap::iterator it=seen.begin(); it(); ++it)
    {
      KGBElt kgb_nr = *it+p.first;
      x_of[kgb_nr]=kgb_nr_of.size();
      kgb_nr_of.push_back(kgb_nr);
      info.push_back(EltInfo(x_of[kgb_nr],0,DescentStatus(),0));
      // makes |info.size()=kgb_nr_of.size()|, |y| number remains 0 in loop
      // the length is set to $0$, its true mathematical value is not known
    }

    // ensure that |element| will find the elements created so far
    compute_first_zs(); // defines |kgb_nr_of.size()+1| values |first_z_of_x|
  } // end of step 3

  // step 4: generate packets for successive involutions

  std::deque<BlockElt> queue(1,info.size()); // involution packet boundaries
  std::vector<KGBElt> ys; ys.reserve(0x100); // |y| indices in invol.packet
  std::vector<KGBElt> cross_ys(ys.size()); cross_ys.reserve(0x100);
  std::vector<KGBElt> Cayley_ys(ys.size()); Cayley_ys.reserve(0x100);

  for (BlockElt next=0; not queue.empty(); next=queue.front(),queue.pop_front())
  { // process involution packet of elements from |next| to |queue.front()|

    const TwistedInvolution tw = kgb.involution(kgb_nr_of[this->x(next)]);
    const InvolutionNbr inv = i_tab.nr(tw); // $\theta$

    ys.clear();
    unsigned int nr_y = 0;
    BlockElt cur_size = info.size();
    for (BlockElt z=next; z<cur_size and this->x(z)==this->x(next); ++z,++nr_y)
    {
      assert(y(z)==y(next)+nr_y); // consecutive, so use of array |ys|
      ys.push_back(y(z));         // could have been avoided
    }

    assert((queue.front()-next)%nr_y==0); // |x| values in equal-size groups
    unsigned int nr_x= (queue.front()-next)/nr_y; // number of such groups

    for (weyl::Generator s=0; s<our_rank; ++s)
    {
      std::vector<block_fields>& tab_s = data[s];
      tab_s.resize(cur_size); // ensure enough slots for now

      // do (non-simple) cross action on |x| and compute new |sub|-length
      int l = length(next);
      KGBElt cross_sample =
	kgb.cross(sub.to_simple(s),kgb_nr_of[this->x(next)]);
      l -= kgb.length(cross_sample); // might become negative
      cross_sample = kgb.cross(sub.simple(s),cross_sample);
      l += kgb.length(cross_sample);
      cross_sample = kgb.cross(cross_sample,sub.to_simple(s));
      assert(l>=0); // if fails, then starting point not minimal for length

      InvolutionNbr s_x_inv = kgb.inv_nr(cross_sample); // $s\times\theta$
      bool new_cross = x_of[cross_sample] == ~0u; // whether involution unseen

      cross_ys.clear();
      { // compute values into |cross_ys|
	size_t old_size =  y_hash.size();
	for (unsigned int j=0; j<nr_y; ++j)
	{
	  TorusElement t = y_hash[ys[j]].t_rep;
	  if (i_tab.complex_roots(inv).isMember(sub.parent_nr_simple(s)))
	    Tg.complex_cross_act(s,t);
	  else if (i_tab.real_roots(inv).isMember(sub.parent_nr_simple(s)))
	    t = t.simple_imaginary_cross(rd,sub.parent_nr_simple(s));
	  /* We could use |simple_imaginary_cross| because |is_valid| has
	     ensured that all coroots of |sub| are integral on |t|, and the
	     root applied is simple-imaginary (and even simple) in |sub|. More
	     prudently one could use |Tg.imaginary_cross_act(s,t);|, which
	     does not depend on integrality, but does assume a simple root */
	  cross_ys.push_back(y_hash.match(i_tab.pack(t,s_x_inv)));
	  assert(y_hash.size()== (new_cross ? old_size+j+1 : old_size));
	  ndebug_use(old_size);
	}
      } // compute values |cross_ys|

      // some variables needed outside next loop, to handle Cayley transforms
      bool new_Cayley=false; // whether Cayley by |s| gave a new involution
      bool first_Cayley=true; // true until Cayley transform for an |x| is done
      size_t y_begin, y_end; // set when |new_Cayley| becomes true

      for (unsigned int i=0; i<nr_x; ++i)
      {
	BlockElt base_z = next+i*nr_y; // first element of R-packet
	KGBElt n = kgb_nr_of[this->x(base_z)]; // nr in |kgb| for current $x$
	KGBElt s_x_n = kgb.cross(sub.reflection(s),n); // its |s|-cross image
	assert (new_cross == (x_of[s_x_n]==(unsigned int)~0));
	// cross action on |x| gives a fresh value iff it did so for the |ys|
	if (new_cross) // then add a new R-packet generated by cross action
	{
	  x_of[s_x_n] = kgb_nr_of.size(); // will be $x$ index for new elements
	  kgb_nr_of.push_back(s_x_n);     // and |s_x_n| will be their kgb nr
	  for (unsigned int j=0; j<nr_y; ++j)
	  {
	    tab_s[base_z+j].cross_image = info.size(); // link to new element

	    info.push_back(EltInfo(x_of[s_x_n],cross_ys[j],DescentStatus(),l));
	    // same |x| neighbour throughout loop, but |y| neighbour varies
	  } // |for(j)|

	  d_first_z_of_x.push_back(info.size()); // finally mark end of R-packet
	} // |if(new_cross)|

	else // just install cross links to previously existing elements
	  for (unsigned int j=0; j<nr_y; ++j)
	    tab_s[base_z+j].cross_image = element(x_of[s_x_n],cross_ys[j]);

	// now compute component |s| of |info[z].descent|, this |x|, all |y|s
	KGBElt conj_n = kgb.cross(sub.to_simple(s),n); // relevant conjugate
	switch(kgb.status(sub.simple(s),conj_n)) // which determines status
	{
	case gradings::Status::Complex:
	  if (kgb.isDescent(sub.simple(s),conj_n))
	    for (unsigned int j=0; j<nr_y; ++j)
	      info[base_z+j].descent.set(s,DescentStatus::ComplexDescent);
	  else
	    for (unsigned int j=0; j<nr_y; ++j)
	      info[base_z+j].descent.set(s,DescentStatus::ComplexAscent);
	  break;
	case gradings::Status::ImaginaryCompact:
	  for (unsigned int j=0; j<nr_y; ++j)
	      info[base_z+j].descent.set(s,DescentStatus::ImaginaryCompact);
	  break;
	case gradings::Status::ImaginaryNoncompact:
	  if (kgb.cross(sub.simple(s),conj_n)!=conj_n)
	    for (unsigned int j=0; j<nr_y; ++j)
	      info[base_z+j].descent.set(s,DescentStatus::ImaginaryTypeI);
	  else
	    for (unsigned int j=0; j<nr_y; ++j)
	      info[base_z+j].descent.set(s,DescentStatus::ImaginaryTypeII);
	  break; // but this case is re-examined below
	case gradings::Status::Real: // now status depends on |y|
	  for (unsigned int j=0; j<nr_y; ++j)
	    if (cross_ys[j] != ys[j]) // implies parity condition
	      info[base_z+j].descent.set(s,DescentStatus::RealTypeII);
	    else
	      if (y_hash[y(base_z+j)].t_rep.negative_at
		  (rd.coroot(sub.parent_nr_simple(s))))
		info[base_z+j].descent.set(s,DescentStatus::RealNonparity);
	      else
		info[base_z+j].descent.set(s,DescentStatus::RealTypeI);
	  break;// but this case is also re-examined, further down below
	} // |switch(kgb.status)|

	  // now do Cayley transform through |s| if applicable
	if (kgb.status(sub.simple(s),conj_n)
	    ==gradings::Status::ImaginaryNoncompact)
	{
	  bool type2= descentValue(s,base_z)==DescentStatus::ImaginaryTypeII;
	  KGBElt s_Cayley_n = kgb::Cayley(kgb,n,sub.simple(s),sub.to_simple(s));

	  l = length(next)+1; // length always increases in Cayley transform

	  if (first_Cayley) // do only once in loop on |i| (but maybe for |i>0|)
	  {
	    first_Cayley = false; // by doing this at most once per involution
	    new_Cayley = x_of[s_Cayley_n]==(unsigned int)~0; // set first time
	    InvolutionNbr Cayley_inv = kgb.inv_nr(s_Cayley_n);

	    if (new_Cayley) // Cayley transform gives an unseen involution
	    { // first ensure all |y|'s for new involution are in hash table

	      y_begin = y_hash.size();
	      TorusElement t = y_hash[ys[0]].t_rep;
	      Tg.do_inverse_Cayley(s,t);
	      y_hash.match(i_tab.pack(t,Cayley_inv));

	      // subsytem real (for parent) cross actions complete set of y's
	      RootNbrList rb =
		rd.simpleBasis(sub.positive_roots() &
			       i_tab.real_roots(Cayley_inv));
	      for (size_t j=y_begin; j<y_hash.size(); ++j) // |y_hash| grows
	      {
		TorusElement t = y_rep(j);
		for (size_t k=0; k<rb.size(); ++k)
		{
		  TorusElement new_t = t.simple_imaginary_cross(rd,rb[k]);
		  y_hash.match(i_tab.pack(new_t,Cayley_inv));
		}
	      }
	      y_end = y_hash.size();
	    } // |if (new_Cayley)|

	    // now fill |Cayley_ys|, whether with elements just created or old
	    Cayley_ys.resize(type2 ? 2*nr_y : nr_y);
	    for (unsigned int j=0; j<nr_y; ++j)
	    {
	      TorusElement t = y_hash[ys[j]].t_rep;
	      Tg.do_inverse_Cayley(s,t); // first Cayley
	      Cayley_ys[j]= y_hash.find(i_tab.pack(t,Cayley_inv));

	      if (type2) // make sure the two Cayley transforms are paired
	      { // second Cayley
		t = t.simple_imaginary_cross(rd,sub.parent_nr_simple(s));
		Cayley_ys[j+nr_y] = y_hash.find(i_tab.pack(t,Cayley_inv));
	      }
	    } // |for (j)|
	    // all |Cayley_ys| are distinct, and in pairs in case of type 2
	  } // |if (first_Cayley)|

	  // all new |y|s are installed in |y_hash| and |Cayley_ys|
	  // so now create the new (x,y) pairs, and Cayley-link to them

	  // next test is not equivalent to |new_Cayley|: Cayley not injective
	  if (x_of[s_Cayley_n]==(unsigned int)~0)
	  {  // then make new element(s)
	    assert(new_Cayley); // if any |x| is new, then first Cayley was

	    x_of[s_Cayley_n] = kgb_nr_of.size(); // created Cayley-ed x
	    kgb_nr_of.push_back(s_Cayley_n);

	    for (size_t j=y_begin; j<y_end; ++j)
	      info.push_back(EltInfo(x_of[s_Cayley_n],j,DescentStatus(),l));

	    d_first_z_of_x.push_back(info.size()); // mark end of R-packet
	  } // |if(x_of[s_x_n]==(unsigned int)~0))|

	  // independently of new creation, make links using |element| lookup
	  for (unsigned int j=0; j<nr_y; ++j)
	  {
	    tab_s[base_z+j].Cayley_image.first =
	      element(x_of[s_Cayley_n],Cayley_ys[j]);
	    if (type2)
	      tab_s[base_z+j].Cayley_image.second =
		element(x_of[s_Cayley_n],Cayley_ys[j+nr_y]);
	  } // |for(j)|
	} // |if(ImaginaryNoncompact)|

	else if (kgb.status(sub.simple(s),conj_n)==gradings::Status::Real)
	{ // install reverse Cayley link(s)
	  KGBElt ctx =
	    kgb::inverse_Cayley(kgb,n,sub.simple(s),sub.to_simple(s));
	  for (unsigned int j=0; j<nr_y; ++j)
	    if (descentValue(s,base_z+j) != DescentStatus::RealNonparity)
	    {
	      TorusElement t = y_hash[ys[j]].t_rep;
	      // Cayley transform leaves |t| unchanged
	      KGBElt cty = y_hash.find(i_tab.pack(t,kgb.inv_nr(ctx)));
	      BlockElt mother = element(x_of[ctx],cty);
	      assert(tab_s[mother].Cayley_image.first ==base_z+j or
		     tab_s[mother].Cayley_image.second==base_z+j); // link here
	      tab_s[base_z+j].Cayley_image.first = mother;
	      BlockElt father = tab_s[mother].cross_image;
	      if (father!=mother) // true in type I
	      {
		assert(descentValue(s,base_z+j) == DescentStatus::RealTypeI);
		tab_s[base_z+j].Cayley_image.second = father;
		assert(tab_s[father].Cayley_image.first
		       ==base_z+j); // single-valued
	      }
	    } // if(not RealNonparity)
	  // |for(j)|
	} // |if (Real)|
      } // |for(i)|
      if (new_cross or new_Cayley)
	queue.push_back(info.size()); // mark end of new involution packet
    } // |for(s)|
  } // |for (next<queue.front())|

  // finish off construction
  std::vector<KGBElt>(kgb_nr_of).swap(kgb_nr_of); // consolidate size

  compute_duals(G);

  // finally look up which element matches the original input
  entry_element = element
    (x_of[x_org],y_hash.find(i_tab.pack(y_org.torus_part(),kgb.inv_nr(x_org))));

} // |gamma_block::gamma_block|

RealReductiveGroup& param_block::realGroup() const
  { return rc.realGroup(); }
const ComplexReductiveGroup& param_block::complexGroup() const
  { return rc.realGroup().complexGroup(); }
const InvolutionTable& param_block::involution_table() const
  { return complexGroup().involution_table(); }

class nblock_elt // internal representation during construction
{
  friend class nblock_help;
  KGBElt xx; // identifies element in parent KGB set
  TorusElement yy; // adds "local system" information to |xx|
public:
  nblock_elt (KGBElt x, const TorusElement& t) : xx(x), yy(t) {}

  KGBElt x() const { return xx; }
  const TorusElement y() const { return yy; }

}; // |class nblock_elt|

class nblock_help // a support class for |nblock_elt|
{
public: // references stored for convenience, no harm in exposing them
  const KGB& kgb;
  const RootDatum& rd;  // the full (parent) root datum
  const SubSystem& sub; // the relevant subsystem
  const InvolutionTable& i_tab; // information about involutions, for |pack|

private:
  std::vector<TorusPart> dual_m_alpha; // the simple roots, reduced modulo 2
  std::vector<TorusElement> half_alpha; // half the simple roots

  void check_y(const TorusElement& t, InvolutionNbr i) const;
  void parent_cross_act(nblock_elt& z, weyl::Generator s) const;
  void parent_up_Cayley(nblock_elt& z, weyl::Generator s) const;
  void parent_down_Cayley(nblock_elt& z, weyl::Generator s) const;

public:
  nblock_help(RealReductiveGroup& GR, const SubSystem& subsys)
    : kgb(GR.kgb()), rd(subsys.parent_datum()), sub(subsys)
    , i_tab(GR.complexGroup().involution_table())
    , dual_m_alpha(kgb.rank()), half_alpha()
  {
    assert(kgb.rank()==rd.semisimpleRank());
    half_alpha.reserve(kgb.rank());
    for (weyl::Generator s=0; s<kgb.rank(); ++s)
    {
      dual_m_alpha[s]=TorusPart(rd.simpleRoot(s));
      half_alpha.push_back(TorusElement(RatWeight(rd.simpleRoot(s),2),false));
    }
  }

  void cross_act(nblock_elt& z, weyl::Generator s) const;
  void cross_act_parent_word(const WeylWord& ww, nblock_elt& z) const;
  void do_up_Cayley (nblock_elt& z, weyl::Generator s) const;
  void do_down_Cayley (nblock_elt& z, weyl::Generator s) const;
  bool is_real_nonparity(nblock_elt z, weyl::Generator s) const; // by value

  y_entry pack_y(const nblock_elt& z) const
  {
    InvolutionNbr i = kgb.inv_nr(z.x());
#ifndef NDEBUG
    check_y(z.y(),i);
#endif
    return i_tab.pack(z.y(),i);
  }
}; // |class nblock_help|

void nblock_help::check_y(const TorusElement& t, InvolutionNbr i) const
{
  InvolutionData id = sub.involution_data(i_tab.matrix(i));
  const RootNbrList& rb = id.real_basis();
  for (unsigned i=0; i<rb.size(); ++i)
    assert(t.evaluate_at(rd.coroot(rb[i])).normalize().denominator()==1);
}

void nblock_help::parent_cross_act(nblock_elt& z, weyl::Generator s) const
{
  switch (kgb.status(s,z.x()))
  {
  case gradings::Status::Complex:
    z.yy.reflect(rd,rd.simpleRootNbr(s));
    break;
  case gradings::Status::Real:
    z.yy.reflect(rd,rd.simpleRootNbr(s));
    z.yy += dual_m_alpha[s];
    break;
  default: {} // nothing for imaginary (and hence real for |z.yy|) roots
  }
  z.xx=kgb.cross(s,z.xx);
}

void nblock_help::cross_act_parent_word(const WeylWord& ww, nblock_elt& z)
  const
{
  for (size_t i=ww.size(); i-->0; )
    parent_cross_act(z,ww[i]);
}

void nblock_help::cross_act (nblock_elt& z, weyl::Generator s) const
{
  cross_act_parent_word(sub.reflection(s),z);
}

void nblock_help::parent_up_Cayley(nblock_elt& z, weyl::Generator s) const
{
  z.xx=kgb.cayley(s,z.xx); // direct Cayley transform on $x$ side

  /* on $y$ side ensure that |z.yy.evaluate_at(rd.simpleCoroot(s))| is even.
   We must adapt by adding a multiple of |simpleRoot(s)|. This may be a
   half-integer multiple even if the initial evaluation is integer, and due to
   that we cannot ensure that the evaluation of |z.yy| on all roots remains
   integer (it will be on real roots, but roots can change their status). In
   the end, adjustment is by a general rational multiple of |simpleRoot(s)|.
  */
  Rational r = z.yy.evaluate_at(rd.simpleCoroot(s))/=2; // in $\Q/\Z$
  int remainder = r.numerator()%r.denominator(); // negative result is OK here
  if (remainder!=0) // odd
    z.yy-=TorusElement(RatWeight(rd.simpleRoot(s)*remainder,r.denominator()),
		       false);
}

void nblock_help::do_up_Cayley (nblock_elt& z, weyl::Generator s) const
{
  const WeylWord& ww=sub.to_simple(s);
  for (size_t i=ww.size(); i-->0; )
    parent_cross_act(z,ww[i]);
  parent_up_Cayley(z,sub.simple(s));
  for (size_t i=0; i<ww.size(); ++i)
    parent_cross_act(z,ww[i]);
}

bool nblock_help::is_real_nonparity(nblock_elt z, weyl::Generator s) const
{
  cross_act_parent_word(sub.to_simple(s),z);
  assert(kgb.status(sub.simple(s),z.x())==gradings::Status::Real);
  Rational r = z.yy.evaluate_at(rd.simpleCoroot(sub.simple(s))); // modulo $2\Z$
  assert(r.numerator()%r.denominator()==0); // should be integer: real coroot
  return (r.numerator()/r.denominator())%2!=0; // return whether odd
}

void nblock_help::parent_down_Cayley(nblock_elt& z, weyl::Generator s) const
{
  z.xx=kgb.inverseCayley(s,z.xx).first; // inverse Cayley transform on $x$ side
  // on $y$ side just keep the same dual |TorusElement|, so nothing to do
#ifndef NDEBUG
  Rational r = z.yy.evaluate_at(rd.simpleCoroot(s)); // modulo $2\Z$
  assert(r.numerator()%(2*r.denominator())==0); // but it must be a parity root
#endif
}

void nblock_help::do_down_Cayley (nblock_elt& z, weyl::Generator s) const
{
  const WeylWord& ww=sub.to_simple(s);
  for (size_t i=ww.size(); i-->0; )
    parent_cross_act(z,ww[i]);
  parent_down_Cayley(z,sub.simple(s));
  for (size_t i=0; i<ww.size(); ++i)
    parent_cross_act(z,ww[i]);
}

BlockElt non_integral_block::element(KGBElt x,KGBElt y) const
{
  return z_hash.find(block_elt_entry(x,y,0)); // length ignored in search
}

void non_integral_block::add_z(KGBElt x,KGBElt y, unsigned short l)
{
  size_t old_size=info.size();
  BlockElt z=z_hash.match(block_elt_entry(x,y,l));
  assert(z==old_size);
  assert(z+1==info.size());
  ndebug_use(old_size);
}

non_integral_block::non_integral_block
  (const Rep_context& rc,
   StandardRepr sr,             // by value; made dominant internally
   BlockElt& entry_element	// set to block element matching input
  )
  : param_block(rc,rootdata::integrality_rank(rc.rootDatum(),sr.gamma()))
  , z_hash(info)
{
  const ComplexReductiveGroup& G = complexGroup();
  const RootDatum& rd = G.rootDatum();
  const InvolutionTable& i_tab = G.involution_table();
  const KGB& kgb = rc.kgb();

  rc.make_dominant(sr); // make dominant before computing subsystem
  infin_char=sr.gamma(); // now we can set the infinitesimal character

  const SubSystem sub = SubSystem::integral(rd,infin_char);

  size_t our_rank = sub.rank(); // this is independent of ranks in |GR|
  for (weyl::Generator s=0; s<our_rank; ++s)
    singular.set(s,rd.coroot(sub.parent_nr_simple(s))
		            .dot(infin_char.numerator())==0);

  nblock_help aux(realGroup(),sub);

  // step 1: get |y|, which has $y.t=\exp(\pi\ii(\gamma-\lambda))$ (vG based)
  const KGBElt x_org = sr.x();
  const nblock_elt org(x_org,y_values::exp_pi(infin_char-rc.lambda(sr)));

  // step 2: move up toward the most split fiber for the current real form
  { // modify |x| and |y|, ascending for |x|, descending for |y|
    nblock_elt z = org;
    weyl::Generator s;
    do
      for(s=0; s<our_rank; ++s)
      {
	KGBElt xx=kgb.cross(sub.to_simple(s),z.x());
	weyl::Generator ss=sub.simple(s);
	if (kgb.isAscent(ss,xx))
	{
	  if (kgb.status(ss,xx)==gradings::Status::Complex)
	    aux.cross_act(z,s);
	  else // imaginary noncompact
	  {
	    assert(kgb.status(ss,xx) == gradings::Status::ImaginaryNoncompact);
	    aux.do_up_Cayley(z,s);
	  }
	  break;
	} // |if(isDescent)|
      } // |for(s)|
    while(s<our_rank); // loop until no descents found in |subsys|

    // DON'T assert(x+1==kgb.size()); fails e.g. in complex groups, empty |sub|

    kgb_nr_of.push_back(z.x()); // save obtained value for |x|
    x_of[z.x()]=0; // and the parent KGB element |x| will get renumbered 0
    y_hash.match(aux.pack_y(z)); // save obtained value for |y|
  } // end of step 2

  // step 3: generate imaginary fiber-orbit of |y|'s (|x| is unaffected)
  {
    const KGBElt x0 = kgb_nr_of[0];
    const InvolutionNbr theta0 = kgb.inv_nr(x0);

    // generating reflections are by subsystem real roots for |theta|
    RootNbrSet pos_real = sub.positive_roots() & i_tab.real_roots(theta0);
    RootNbrList gen_root = rd.simpleBasis(pos_real);
    for (size_t i=0; i<y_hash.size(); ++i) // |y_hash| grows
    {
      const TorusElement y = y_hash[i].repr();
      for (weyl::Generator s=0; s<gen_root.size(); ++s)
      {
	nblock_elt z(x0,y);
	aux.cross_act_parent_word(rd.reflectionWord(gen_root[s]),z);
	assert(z.x()==x0);
	y_hash.match(aux.pack_y(z));
      }
    }

    // now insert elements from |yy_hash| as first R-packet of block
    info.reserve(y_hash.size()); // this is lower bound for final size; reserve
    for (weyl::Generator s=0; s<our_rank; ++s)
      data[s].reserve(y_hash.size());

    for (size_t i=0; i<y_hash.size(); ++i)
      add_z(0,i,0); // length is on dual side

    // we leave |first_z_of_x| empty, |compute_first_zs()| would set 2 values
  } // end of step 3

  // step 4: generate packets for successive involutions

  std::vector<BlockElt> queue(1,info.size()); // involution packet boundaries
  std::vector<KGBElt> ys; ys.reserve(0x100); // enough for |1<<RANK_MAX|
  std::vector<KGBElt> cross_ys(ys.size()); cross_ys.reserve(0x100);
  std::vector<KGBElt> Cayley_ys(ys.size()); Cayley_ys.reserve(0x100);

  size_t qi=0; // index into queue

  for (BlockElt next=0; qi<queue.size(); next=queue[qi++])
  { // process involution packet of elements from |next| to |queue[qi]|

    const KGBElt first_x = parent_x(next);
    const InvolutionNbr i_theta = kgb.inv_nr(first_x);

    ys.clear();
    size_t nr_y = 0;
    // now traverse R_packet of |first_x|, collecting their |y|'s
    for (BlockElt z=next; z<info.size() and x(z)==x(next); ++z,++nr_y)
    {
      assert(y_hash[y(z)].nr==i_theta); // involution of |y| must match |x|
      assert(y(z)==y(next)+nr_y);   // and |y|s are consecutive
      ys.push_back(y(z));           // so |ys| could have been avoided
    }

    assert((queue[qi]-next)%nr_y==0); // |x| values in equal-size R-packets
    unsigned int nr_x= (queue[qi]-next)/nr_y; // number of R-packets here

    for (weyl::Generator s=0; s<our_rank; ++s)
    {
      std::vector<block_fields>& tab_s = data[s];
      tab_s.resize(info.size()); // ensure enough slots for now

      const RootNbr alpha=sub.parent_nr_simple(s); // root currently considered
      unsigned int y_start=y_hash.size(); // new |y|s numbered from here up

      // compute length change; only nonzero for complex roots; if so, if
      // $\theta(\alpha)$ positive, like $\alpha$, then go down (up for $x$)
      int d = i_tab.complex_roots(i_theta).isMember(alpha)
	    ? rd.isPosRoot(i_tab.root_involution(i_theta,alpha)) ? -1 : 1
	    : 0 ;
      int length = this->length(next) + d;
      assert(length>=0); // if not, then starting point not minimal for length

      nblock_elt sample(first_x,y_hash[ys[0]].repr());
      aux.cross_act(sample,s);
      bool new_cross = // whether cross action discovers unseen involution
	y_hash.find(aux.pack_y(sample)) == y_hash.empty;
      assert(new_cross == (x_of[sample.x()]==(unsigned int)~0));
      // cross action gives a fresh |x| coordinate iff it gives fresh |y|

      cross_ys.clear(); Cayley_ys.clear();
      { // compute values into |cross_ys|
	size_t old_size =  y_hash.size();
	for (unsigned int j=0; j<nr_y; ++j)
	{
	  nblock_elt z(first_x,y_hash[ys[j]].repr());
	  aux.cross_act(z,s);
	  cross_ys.push_back(y_hash.match(aux.pack_y(z)));
	  assert(y_hash.size()== (new_cross ? old_size+j+1 : old_size));
	  ndebug_use(old_size);
	}
      } // compute values |cross_ys|

      for (unsigned int i=0; i<nr_x; ++i)
      {
	BlockElt base_z = next+i*nr_y; // first element of R-packet
	KGBElt n = parent_x(base_z);
	KGBElt s_x_n = kgb.cross(sub.reflection(s),n); // don't need |y| here
	assert (new_cross == (x_of[s_x_n]==(unsigned int)~0));
	// cross action on |n| gives a fresh value iff it did for the |ys|

	// set the cross links for this |n| and all corresponding |ys|
	if (new_cross) // add a new R-packet
	{
	  x_of[s_x_n] = kgb_nr_of.size(); // install a new |x| value that
	  kgb_nr_of.push_back(s_x_n); // corresponds to parent element |s_x_n|
	  for (unsigned int j=0; j<nr_y; ++j)
	  {
	    tab_s[base_z+j].cross_image = info.size(); // link to new element

	    add_z(x_of[s_x_n],cross_ys[j],length);
	    // same |x| neighbour throughout loop, but |y| neighbour varies
	  } // |for(j)|
	  // |d_first_z_of_x.push_back(info.size())| would mark end of R-packet
	} // |if(new_cross)|
	else // install cross links to previously existing elements
	  for (unsigned int j=0; j<nr_y; ++j)
	    tab_s[base_z+j].cross_image = element(x_of[s_x_n],cross_ys[j]);

	// compute component |s| of |info[z].descent|, this |n|, all |y|s
	KGBElt conj_n = kgb.cross(sub.to_simple(s),n); // conjugate
	bool do_Cayley=false; // is made |true| if a real parity case is found
	switch(kgb.status(sub.simple(s),conj_n))
	{
	case gradings::Status::Complex:
	  if (kgb.isDescent(sub.simple(s),conj_n))
	    for (unsigned int j=0; j<nr_y; ++j)
	      info[base_z+j].descent.set(s,DescentStatus::ComplexDescent);
	  else
	    for (unsigned int j=0; j<nr_y; ++j)
	      info[base_z+j].descent.set(s,DescentStatus::ComplexAscent);
	  break;
	case gradings::Status::ImaginaryCompact:
	  for (unsigned int j=0; j<nr_y; ++j)
	      info[base_z+j].descent.set(s,DescentStatus::ImaginaryCompact);
	  break;
	case gradings::Status::ImaginaryNoncompact:
	  if (kgb.cross(sub.simple(s),conj_n)!=conj_n)
	    for (unsigned int j=0; j<nr_y; ++j)
	      info[base_z+j].descent.set(s,DescentStatus::ImaginaryTypeI);
	  else
	    for (unsigned int j=0; j<nr_y; ++j)
	      info[base_z+j].descent.set(s,DescentStatus::ImaginaryTypeII);
	  break;
	case gradings::Status::Real: // now status depends on |y|
	  for (unsigned int j=0; j<nr_y; ++j)
	  {
	    nblock_elt z(n,y_hash[ys[j]].repr());
	    if (aux.is_real_nonparity(z,s))
	      info[base_z+j].descent.set(s,DescentStatus::RealNonparity);
	    else
	    {
	      do_Cayley=true;
	      if (cross_ys[j] != ys[j])
		info[base_z+j].descent.set(s,DescentStatus::RealTypeII);
	      else
		info[base_z+j].descent.set(s,DescentStatus::RealTypeI);
	    }
	  }
	  break;// but this case is re-examined, below
	} // |switch(kgb.status)|


	// now do inverse Cayley transform through |s| if applicable
	if (do_Cayley)
	{
	  KGBEltPair Cayleys = kgb.inverseCayley(sub.simple(s),conj_n);
	  KGBElt ctx1 = kgb.cross(Cayleys.first,sub.to_simple(s));
	  length = this->length(next)+1; // length always increases here

	  if (i==0)
	  { // |do_Cayley| is independent of |x|, if so, do first time:
	    assert(y_hash.size()==y_start); // nothing created by cross actions
	    KGBElt x_start = kgb_nr_of.size();

	    bool new_Cayley = (x_of[ctx1]==(unsigned int)~0);

	    // independent of new creation, fill |Cayley_ys|, extend |y_hash|
	    for (unsigned int j=0; j<nr_y; ++j)
	      if (descentValue(s,base_z+j) != DescentStatus::RealNonparity)
	      {
		nblock_elt z(n,y_hash[ys[j]].repr());
		aux.do_down_Cayley(z,s);
		Cayley_ys.push_back(y_hash.match(aux.pack_y(z)));
	      }

	    if (new_Cayley) // then we need to create new R-packets
	    {
	      // first create the set of |x| values giving R-packets
	      x_of[ctx1] = x_start; // create |x| for the first new R-packet
	      kgb_nr_of.push_back(ctx1);

	      // complete fiber of x's over new involution using
	      // subsystem imaginary cross actions
              RootNbrSet pos_imag = // subsystem positive imaginary roots
		sub.positive_roots() & i_tab.imaginary_roots(kgb.inv_nr(ctx1));
	      RootNbrList ib = rd.simpleBasis(pos_imag);
	      for (size_t k=x_start; k<kgb_nr_of.size(); ++k) // grows
	      {
		KGBElt cur_x = kgb_nr_of[k];
		for (size_t r=0; r<ib.size(); ++r)
		{
		  KGBElt new_x = kgb.cross(rd.reflectionWord(ib[r]),cur_x);
		  if (x_of[new_x] == (unsigned int)~0)
		  {
		    x_of[new_x] = kgb_nr_of.size();
		    kgb_nr_of.push_back(new_x);
		  }
		} // |for(r)|
	      } // |for (k)|

	      // then generate corrsponding part of block, combining (x,y)'s
	      for (size_t k=x_start; k<kgb_nr_of.size(); ++k)
		for (unsigned int y=y_start; y<y_hash.size(); ++y)
		  add_z(k,y,length);
	        // again we could (and used to) mark end of R-packet here
	      // |for(k)|
	      // finallly make sure that Cayley links slots exist for code below
	      tab_s.resize(info.size());
	    } // |if (new_Cayley)|: finished creating new R-packets
	  } // |if (i==0)|: finished work for first |x| when some |y| is parity

	  // now in all cases make links using |element| lookup
	  for (unsigned int j=0,p=0; j<nr_y; ++j) // |p counts parity |y|s only
	    if (descentValue(s,base_z+j)!=DescentStatus::RealNonparity)
	    {
	      KGBElt cty=Cayley_ys[p++]; // unique Cayley transform of |y|
	      BlockElt target = element(x_of[ctx1],cty);
	      tab_s[base_z+j].Cayley_image.first = target;
	      first_free_slot(tab_s[target].Cayley_image) = base_z+j;
	      if (Cayleys.second!=UndefKGB) // then double valued (type1)
	      {
		KGBElt ctx2 = kgb.cross(Cayleys.second,sub.to_simple(s));
		assert (x_of[ctx2]!=(unsigned int)~0);
		target = element(x_of[ctx2],cty);
		tab_s[base_z+j].Cayley_image.second = target;
		first_free_slot(tab_s[target].Cayley_image) = base_z+j;
	      }
	  } // |for(j)|

	} // |if(do_Cayley)|

      } // |for(i)

      if (y_hash.size()>y_start)
	queue.push_back(info.size()); // mark end of new involution packet
    } // |for(s)|
  } // |for (next<queue[qi])|
  // end of step 4

  // correct for reverse order construction
  { size_t max_l=length(size()-1);
    for (BlockElt z=0; z<size(); ++z)
      info[z].length = max_l-length(z);
  }

  { // renumber the |x| values by (now) increasing length, and reorder block
    std::vector<KGBElt> new_x;
    { KGBElt stop=xsize(); // we shall reunmber the |x| values
      new_x.reserve(stop);
      for (unsigned i=0; i<queue.size(); ++i) // do involution packets in order
      {
	KGBElt start=queue[i]<info.size() ? xsize()-x(queue[i]) : 0;
	for (KGBElt j=start; j<stop; ++j) // do |x| values in a packet
	  new_x.push_back(j); // allocate slice of new values, lower than before
	stop=start; // next slice will stop where this one started
      }
    }

    renumber_x(new_x); // does the actual renumbering, and reordering of block
    Permutation pi(new_x.begin(),new_x.end());
    pi.permute(kgb_nr_of);
    pi.renumber(x_of,UndefKGB); // renumber, leaving |UndefKGB| as is

    z_hash.reconstruct(); // adapt to permutation and remapped |x| values
  }

  compute_duals(G); // finally compute Hermitian duals

  // and look up which element matches the original input
  entry_element = element(x_of[x_org],y_hash.find(aux.pack_y(org)));

} // |non_integral_block::non_integral_block|




struct partial_nblock_help : public nblock_help
{
  y_part_hash& y_hash; // will reference a field of |param_block|
  block_hash& z_hash;  // will reference a field of |non_integral_block|

  std::vector<BlockEltList> predecessors; // a list of predecessors for each z

  partial_nblock_help
    (RealReductiveGroup& GR, const SubSystem& subsys,
     y_part_hash& hy, block_hash& hz)
  : nblock_help(GR,subsys)
  , y_hash(hy)
  , z_hash(hz)
  , predecessors()
{}

  KGBElt conj_in_x(weyl::Generator s,KGBElt x) const
  { return kgb.cross(sub.to_simple(s),x); }
  KGBElt conj_out_x(KGBElt x,weyl::Generator s) const
  { return kgb.cross(x,sub.to_simple(s)); }

  unsigned int length(BlockElt z_inx) const { return z_hash[z_inx].length; }

  nblock_elt get(BlockElt z_inx) const
  {
    const block_elt_entry& e=z_hash[z_inx];
    return nblock_elt(e.x,y_hash[e.y].repr());
  }
  BlockElt lookup(const block_elt_entry& z_entry) const
  { return z_hash.find(z_entry); }

  BlockElt lookup(KGBElt x,KGBElt y) const
  { return lookup(block_elt_entry(x,y,0)); }

  BlockElt lookup(const nblock_elt& z) const
  { KGBElt y = y_hash.find(pack_y(z));
    if (y==y_hash.empty)
      return z_hash.empty;
    else return lookup(z.x(),y);
  }

  BlockElt nblock_below (const nblock_elt& z);

}; // |class partial_nblock_help|

// |nblock_below| extends |y_hash|, and |z_hash| with |z|, having ensured the
// presence of its predecessors. Returns (new, current max) hash index of |z|
BlockElt partial_nblock_help::nblock_below (const nblock_elt& z)
{
  { // check if already known, but don't add to |z_hash| yet if not
    BlockElt res = lookup(z);
    if (res!=z_hash.empty)
      return res;
  }

  BlockEltList pred; // will hold list of elements covered by z

  weyl::Generator s; // a complex or real type 1 descent, if such exists

  // |sz_inx| will take a value depending on |s|, but must survive |break|
  BlockElt sz_inx; // will hold number returned by recursive call
  for (s=0; s<sub.rank(); ++s)
  {
    nblock_elt sz = z; // have a copy ready for modification
    KGBElt conj_x= conj_in_x(s,z.x());
    if (kgb.isComplexDescent(sub.simple(s),conj_x))
    {
      cross_act(sz,s);
      sz_inx = nblock_below(sz);
      pred.reserve(predecessors[sz_inx].size()+1); // a rough estimate
      pred.push_back(sz_inx); // certainly |sz| is predecessor of |z|
      break; // we shall add $s$-ascents of |predecessors[sz_inx]| below
    }
    else if (kgb.isDoubleCayleyImage(sub.simple(s),conj_x)) // excludes type 2
    {
      if (not is_real_nonparity(z,s)) // excludes real nonparity
      { // so we now know that |z| has a type 1 real descent at |s|
	do_down_Cayley(sz,s);
 	sz_inx = nblock_below(sz);
	pred.reserve(predecessors[sz_inx].size()+2); // a rough estimate
	pred.push_back(sz_inx);
	cross_act(sz,s); // get other inverse Cayley image of |z|
	pred.push_back(nblock_below(sz)); // and include it in |pred|
	break; // we shall add $s$-ascents of |predecessors[sz_inx]| below
      } // |if (real_parity)|
    } // |if (doubleCayleyImage)|

  } // |for (s)|

  // if above loop completed, there are no complex or real type I descents
  if (s==sub.rank()) // only real type II descents, insert and return those
  {
    pred.reserve(sub.rank()); // enough, and probably more than that
    while (s-->0) // we reverse the loop just because it looks cute
    {
      nblock_elt sz = z; // have a copy ready for modification
      KGBElt conj_x= conj_in_x(s,z.x());
      if (kgb.status(sub.simple(s),conj_x)==gradings::Status::Real)
      {
	if (not is_real_nonparity(z,s)) // then it was real type II
	{
	  assert (not kgb.isDoubleCayleyImage(sub.simple(s),conj_x));
	  do_down_Cayley(sz,s);
	  pred.push_back(nblock_below(sz)); // recurr, but ignore descents
	}
      }
    } // |while (s-->0)|
  } // |if (s==sub.rank())|
  else // add all |s|-ascents for elements covered by |sz|
    for (BlockElt i=0; i<predecessors[sz_inx].size(); ++i)
    {
      nblock_elt c = get(predecessors[sz_inx][i]);
      KGBElt conj_x= conj_in_x(s,c.x());
      switch (kgb.status(sub.simple(s),conj_x))
      {
      case gradings::Status::Real: case gradings::Status::ImaginaryCompact:
	break; // nothing to do without ascent
      case gradings::Status::Complex:
	if (not kgb.isDescent(sub.simple(s),conj_x)) // complex ascent
	{
	  cross_act(c,s);
	  pred.push_back(nblock_below(c));
	} // |if(complex ascent)
	break;
      case gradings::Status::ImaginaryNoncompact:
	{
	  bool type_2 = kgb.cross(sub.simple(s),conj_x)==conj_x;
	  do_up_Cayley(c,s);
	  pred.push_back(nblock_below(c));

	  if (type_2)
	  {
	    cross_act(c,s); // this should change: we are in type 2
	    pred.push_back(nblock_below(c));
	  }
	}
	break;
      } // |switch(status(s,conj_x))|
    } // |for (i)|

  // finally we can add |z| to |z_hash|, after all its Bruhat-predecessors
  assert(z_hash.size()==predecessors.size());
  BlockElt res = z_hash.match
    (block_elt_entry(z.x(),y_hash.match(pack_y(z)),
		     pred.size()==0 ? 0 : length(pred[0])+1));
  assert(res==predecessors.size()); // |z| must have been added just now
  predecessors.push_back(pred); // store list of elements covered by |z|
  return res;
} // |partial_nblock_help::nblock_below|


// alternative constructor, for interval below |sr|
non_integral_block::non_integral_block
(const Rep_context& rc, StandardRepr sr) // by value; made dominant internally
  : param_block(rc,rootdata::integrality_rank(rc.rootDatum(),sr.gamma()))
  , z_hash(info)
{
  const RootDatum& rd = complexGroup().rootDatum();
  const KGB& kgb = rc.kgb();

  rc.make_dominant(sr); // make dominant before computing subsystem
  infin_char=sr.gamma(); // now we can set the infinitesimal character

  const SubSystem sub = SubSystem::integral(rd,infin_char);

  size_t our_rank = sub.rank(); // this is independent of ranks in |GR|
  for (weyl::Generator s=0; s<our_rank; ++s)
    singular.set(s,rd.coroot(sub.parent_nr_simple(s))
			    .dot(infin_char.numerator())==0);

  partial_nblock_help aux(realGroup(),sub,y_hash,z_hash);

  // step 1: get |y|, which has $y.t=\exp(\pi\ii(\gamma-\lambda))$ (vG based)
  const KGBElt x_org = sr.x();
  const nblock_elt org(x_org,y_values::exp_pi(infin_char-rc.lambda(sr)));

  BlockElt last=aux.nblock_below(org); // generate partial block in |aux|

  {
    std::vector<unsigned int> len(size());
    for (BlockElt z=0; z<size(); ++z)
      len[z]=length(z);
    Permutation pi = permutations::standardization(len,len[last]+1);
    pi.permute(info);
    z_hash.reconstruct(); // adapt to permutation of entries
  }

  size_t size= last+1;
  assert(info.size()==size); // |info| should have obtained precisely this size

  data.assign(our_rank,std::vector<block_fields>(size)); // |UndefBlock| entries

  for (BlockElt i=0; i<size; ++i)
  {
    const block_elt_entry& z=z_hash[i];

    KGBElt x = z.x;
    x_of[x]=x; // record presence of |x|, |kgb_nr_of| remains empty for now

    DescentStatus& desc_z = info[i].descent;
    for (weyl::Generator s=0; s<our_rank; ++s)
    {
      std::vector<block_fields>& tab_s = data[s];
      nblock_elt cur = aux.get(i);

      KGBElt conj_x = aux.conj_in_x(s,x);
      if (kgb.isDescent(sub.simple(s),conj_x))
      {
	if (kgb.status(sub.simple(s),conj_x)==gradings::Status::Complex)
	{
	  aux.cross_act(cur,s);
	  BlockElt sz = aux.lookup(cur);
	  assert(sz!=aux.z_hash.empty);
	  tab_s[i].cross_image = sz; tab_s[sz].cross_image = i;
	  assert(aux.length(sz)+1==length(i));
	  desc_z.set(s,DescentStatus::ComplexDescent);
	  assert(descentValue(s,sz)==DescentStatus::ComplexAscent);
	}
	else // |s| is a real root
	{
	  assert(kgb.status(sub.simple(s),conj_x)==gradings::Status::Real);

	  if (aux.is_real_nonparity(cur,s))
	  {
	    tab_s[i].cross_image = i;
	    desc_z.set(s,DescentStatus::RealNonparity);
	  }
	  else // |s| is real parity
	  {
	    aux.do_down_Cayley(cur,s);
	    BlockElt sz = aux.lookup(cur);
	    assert(aux.length(sz)+1==length(i));
	    tab_s[i].Cayley_image.first = sz; // first inverse Cayley
	    if (kgb.isDoubleCayleyImage(sub.simple(s),conj_x)) // real type 1
	    {
	      desc_z.set(s,DescentStatus::RealTypeI);
	      assert(descentValue(s,sz)==DescentStatus::ImaginaryTypeI);
	      tab_s[i].cross_image = i;
	      tab_s[sz].Cayley_image.first = i; // single-valued Cayley
	      aux.cross_act(cur,s);
	      sz = aux.lookup(cur);
	      assert(descentValue(s,sz)==DescentStatus::ImaginaryTypeI);
	      assert(aux.length(sz)+1==length(i));
	      tab_s[i].Cayley_image.second = sz; // second inverse Cayley
	      tab_s[sz].Cayley_image.first = i;  // single-valued Cayley
	    }
	    else // real type 2
	    {
	      desc_z.set(s,DescentStatus::RealTypeII);
	      assert(descentValue(s,sz)==DescentStatus::ImaginaryTypeII);
	      first_free_slot(tab_s[sz].Cayley_image) // double-valued Cayley
		= i;
	      cur = aux.get(i); // reset to current element
	      aux.cross_act(cur,s);
	      BlockElt cross_z = aux.lookup(cur);
	      if (cross_z!=aux.z_hash.empty) // cross neighbour might be absent
		tab_s[i].cross_image = cross_z;
	    } // type 2
	  } // real parity

	} // |s| is real
      } // |if(isDescent)|
      else if (kgb.status(sub.simple(s),conj_x)==gradings::Status::Complex)
	desc_z.set(s,DescentStatus::ComplexAscent);
      // cross link will be set if and when complex ascent appears in block
      else // imaginary
      {
	if (kgb.status(sub.simple(s),conj_x)
	    == gradings::Status::ImaginaryCompact)
	{
	  desc_z.set(s,DescentStatus::ImaginaryCompact);
	  tab_s[i].cross_image = i;
	}
	else if (kgb.cross(sub.simple(s),conj_x)==conj_x)
	{
	  desc_z.set(s,DescentStatus::ImaginaryTypeII);
	  tab_s[i].cross_image = i;
	}
	else
	{ // In imaginary type 1 situation |z| has a nontrivial cross action
	  desc_z.set(s,DescentStatus::ImaginaryTypeI);
	  KGBElt cross_x = aux.conj_out_x(kgb.cross(sub.simple(s),conj_x),s);
	  BlockElt sz=aux.lookup(cross_x,z.y);
          if (sz!=aux.z_hash.empty) // cross neighbour might be absent
	    tab_s[i].cross_image = sz;
	}
      }
    } // |for(s)|
  } // |for(i)|

  kgb_nr_of.assign(&x_of[0],&x_of[x_org+1]); // copy identity map with holes
  compute_duals(complexGroup());

} // |non_integral_block::non_integral_block|, partial version

RatWeight non_integral_block::y_part(BlockElt z) const
{
  RatWeight t =  y_rep(y(z)).log_pi(false);
  InvolutionNbr i_x = rc.kgb().inv_nr(parent_x(z));
  involution_table().real_unique(i_x,t);
  return (t/=2).normalize();
}


hBlock::hBlock (const KGB& kgb, const KGB& dual_kgb)
  :  Block(kgb,dual_kgb)
  , d_twist(size())
  , d_fixnbr(size(),UndefBlock)
  , d_h()
  , d_hcross(rank()/2)
{
  d_twist.push_back(0);
  d_h.push_back(0);
  d_fixnbr[0] = 0;
  if (size() > 1)
    for (size_t z=1; z<size(); ++z)
    {
      weyl::Generator s=firstStrictGoodDescent(z); //find a descent
      assert(s < hrank());
      assert(isStrictAscent(hrank()+s,d_twist[cross(s,z)]));

      // cross down, twist, cross up
      d_twist[z] = cross(hrank() + s,d_twist[cross(s,z)]);
      if (d_twist[z]==z)
      {
	d_fixnbr[z]=d_h.size();
	d_h.push_back(z);
      }
    }

  for (weyl::Generator s=0; s<hrank(); ++s)
    d_hcross[s].reserve(hsize());

  for (size_t z=0;z<hsize(); ++z) // fill the hcross table
    for (weyl::Generator s=0; s<hrank(); ++s)
    {
      BlockElt yleft=cross(s,d_h[z]);
      BlockElt yright=cross(hrank()+s,d_h[z]);
      if (yleft==yright)
      {
	d_hcross[s].push_back(d_fixnbr[yleft]);
	assert(d_twist[yleft]==yleft);
	assert(length(yleft)==length(d_h[z])+1 or
	       length(yleft)==length(d_h[z])-1);
      }
      else
      {
	d_hcross[s].push_back(d_fixnbr[cross(s,yright)]);
	assert(d_twist[cross(s,yright)]==cross(s,yright));
	assert(length(cross(s,yright))==length(d_h[z])+2 or
	       length(cross(s,yright))==length(d_h[z])-2);
      }
    } // |for (s)|
} // |hBlock::hBlock|

/*****************************************************************************

        Chapter III -- Functions local to blocks.cpp

******************************************************************************/

namespace {

DescentStatus descents(KGBElt x, KGBElt y,
		       const KGB_base& kgb, const KGB_base& dual_kgb)
{
  DescentStatus result;
  for (size_t s = 0; s < kgb.rank(); ++s)
    if (kgb.status(s,x) == gradings::Status::Complex) // s is complex
      if (kgb.isDescent(s,x))
	result.set(s,DescentStatus::ComplexDescent);
      else
	result.set(s,DescentStatus::ComplexAscent);
    else if (kgb.status(s,x) == gradings::Status::ImaginaryNoncompact)
      if (kgb.cross(s,x)!=x) // type I
	result.set(s,DescentStatus::ImaginaryTypeI);
      else // type II
	result.set(s,DescentStatus::ImaginaryTypeII);
    else if (dual_kgb.status(s,y) == gradings::Status::ImaginaryNoncompact)
      if (dual_kgb.cross(s,y)!=y) // type II
	result.set(s,DescentStatus::RealTypeII);
      else // type I
	result.set(s,DescentStatus::RealTypeI);
    // now s is imaginary compact or real nonparity
    else if (kgb.status(s,x) == gradings::Status::Real)
      result.set(s,DescentStatus::RealNonparity);
    else // now |kgb.status(s,x) == gradings::Status::ImaginaryCompact|
      result.set(s,DescentStatus::ImaginaryCompact);

  return result;
}

/*!
  \brief Inserts into |hs| the ascents through |s| from elements of |hr|.

  Explanation: technical function for the Hasse construction, that makes the
  part of the coatom list for a given element arising from a given descent.
*/
void insertAscents(std::set<BlockElt>& hs,
		   const set::EltList& hr,
		   size_t s,
		   const Block_base& block)
{
  for (size_t j = 0; j < hr.size(); ++j)
  {
    BlockElt z = hr[j];
    switch (block.descentValue(s,z))
    {
    case DescentStatus::ComplexAscent:
      hs.insert(block.cross(s,z));
      break;
    case DescentStatus::ImaginaryTypeI:
      hs.insert(block.cayley(s,z).first);
      break;
    case DescentStatus::ImaginaryTypeII:
      hs.insert(block.cayley(s,z).first);
      hs.insert(block.cayley(s,z).second);
      break;
    default: // not a strict ascent
      break;
    }
  }
}


/*!
  Puts into |Hasse| the Hasse diagram data for the Bruhat ordering on |block|.

  Explanation: we used the algorithm from Vogan's 1982 Park City notes...
  which contains a bad definition. Now modified to work like kgb makeHasse:
  seek an ascent s that is complex or type I real. If it exists, use it as in
  kgb. If it doesn't then we're essentially at a split principal series. The
  immediate predecessors of z are just the inverse Cayley transforms.
*/
std::vector<set::EltList> makeHasse(const Block_base& block)
{
  std::vector<set::EltList> result(block.size());

  for (BlockElt z = 0; z < block.size(); ++z)
  {
    std::set<BlockElt> h_z;

    size_t s=block.firstStrictGoodDescent(z);
    if (s<block.rank())
      switch (block.descentValue(s,z))
      {
      default: assert(false); break;
      case DescentStatus::ComplexDescent:
	{
	  BlockElt sz = block.cross(s,z);
	  h_z.insert(sz);
	  insertAscents(h_z,result[sz],s,block);
	}
	break;
      case DescentStatus::RealTypeI: // inverseCayley(s,z) two-valued
	{
	  BlockEltPair sz = block.inverseCayley(s,z);
	  h_z.insert(sz.first);
	  h_z.insert(sz.second);
	  insertAscents(h_z,result[sz.first],s,block);
	}
      }
    else // now just gather all RealTypeII descents of |z|
      for (size_t s = 0; s < block.rank(); ++s)
	if (block.descentValue(s,z)==DescentStatus::RealTypeII)
	  h_z.insert(block.inverseCayley(s,z).first);

    std::copy(h_z.begin(),h_z.end(),std::back_inserter(result[z])); // set->list
  } // for |z|

  return result;
} // |makeHasse|

} // |namespace|

/*****************************************************************************

      Chapter IV -- Functions exported from blocks.cpp (declared in blocks.h)

******************************************************************************/


//!\brief Returns the twisted involution dual to |w|.

/*
  We have $\tau = w.\delta$, with $w$ in the Weyl group $W$, and $\delta$ the
  fundamental involution of $X^*$. We seek the twisted involution $v$ in the
  dual twisted Weyl group |dual_W| such that $-\tau^t = v.\delta^\vee$. Here
  $\delta^\vee$ is the dual fundamental involution, which acts on $X_*$ as
  minus the transpose of the longest involution $w_0.\delta$, where $w_0$ is
  the longest element of $W$. This relation relation can be defined
  independently of being a twisted involution, and leads to a bijection $f$:
  $W \to W$ that is characterised by $f(e)=w_0$ and $f(s.w)=f(w)dwist(s)$ for
  any simple generator $e$ and $w\in W$, where $dwist$ is the twist of the
  dual twisted Weyl group (one also has $f(w.twist(s))=s.f(w)$ so that $f$
  intertwines twisted conjugation: $f(s.w.twist(s))=s.f(w).dwist(s)$).

  The implementation below is based directly on the above characterisation, by
  converting |w| into a |WeylWord|, and then starting from $w_0$
  right-multiplying successively by the $dwist$ image of the letters of the
  word taken right-to-left. The only thing assumed common to |W| and |W_dual|
  is the \emph{external} numbering of generators (letters in |ww|), a minimal
  requirement without which the notion of dual twisted involution would make
  no sense. Notably the result will be correct (when interpreted for |dual_W|)
  even if the underlying (untwisted) Weyl groups of |W| and |dual_W| should
  differ in their external-to-internal mappings. If one assumes that |W| and
  |dual_W| share the same underlying Weyl group (as is currently the case for
  an inner class and its dual) then one could alternatively say simply

    |return W.prod(W.weylGroup().longest(),dual_W.twisted(W.inverse(w)))|

  or

    |return W.prod(W.inverse(W.twisted(w)),W.weylGroup().longest())|.

  Note that this would involve implicit conversion of an element of |W| as one
  of |dual_W|.x
*/
TwistedInvolution
dual_involution(const TwistedInvolution& w,
		const TwistedWeylGroup& W,
		const TwistedWeylGroup& dual_W)
{
  WeylWord ww= W.word(w);
  TwistedInvolution result = dual_W.weylGroup().longest();
  for (size_t i=ww.size(); i-->0; )
    dual_W.mult(result,dual_W.twisted(ww[i]));
  return result;
}


std::vector<BlockElt> dual_map(const Block_base& b, const Block_base& dual_b)
{
  assert(b.size()==dual_b.size());

  std::vector<BlockElt> result(b.size());
  for (BlockElt i=0; i<b.size(); ++i)
    result[i]=dual_b.element(b.y(i),b.x(i));

  return result;
}

BitMap common_Cartans(RealReductiveGroup& GR,
			      RealReductiveGroup& dGR)
  { return GR.Cartan_set()
      & GR.complexGroup().dual_Cartan_set(dGR.realForm());
  }

} // namespace blocks

} // namespace atlas
