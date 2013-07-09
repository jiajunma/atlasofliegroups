
/*!
\file
\brief Implementation for the class ComplexReductiveGroup.

  The ComplexReductiveGroup class will play a central role in the
  whole program.  Even though it is entirely defined by its based root
  datum and an involutive automorphism of that datum, it has seemed
  more natural to use this class to collect the wealth of
  combinatorial data that the root datum gives rise to, and that will
  serve as the basis for our description of the representation theory
  of G. Note that the current state of the theory, and most notably
  Vogan duality, makes it natural and necessary to consider all the
  real forms of our complex group (in a given inner class) at once; so
  that is another reason to not choose a real form a priori.
*/
/*
  This is complexredgp.cpp.

  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Lie Groups and Representations

  For license information see the LICENSE file
*/

#include "complexredgp.h"

#include <set>

#include "weyl.h"


/*****************************************************************************

  The ComplexReductiveGroup class will play a central role in the whole
  program; even though it is entirely defined by its root datum, it has
  seemed more natural to use this class to collect the wealth of
  combinatorial data that the root datum gives rise to, and that will
  serve as the basis for our description of the representation theory
  of G. Note that the current state of the theory, and most notably
  Vogan duality, makes it natural and necessary to consider all the
  real forms of our complex group (in a given inner class) at once;
  so that is another reason to not choose a real form a priori.

  This module, together with the cartanclass one, contains code for
  dealing with conjugacy classes of Cartan subgroups, and real
  forms. In this version, we have de-emphasized the classification of
  strong real forms, which is perhaps better treated as an
  input-output issue (it can certainly be recovered with a moderate
  effort from the data available here.) It appears that the data
  recorded here is what's most relevant to representation theory.

  The enumeration of real forms amounts to that of W^\delta- orbits in the
  fundamental fiber of the one-sided parameter space for the adjoint group
  (see the "combinatorics" paper on the Atlas website, or the forthcoming
  "algorithms" paper by Jeff Adams and Fokko du Cloux); this is a very small
  computation, depending only on the Lie algebra (|RootSystem| only).

  The enumeration of conjugacy classes of Cartan subgroups, for the various
  real forms, is part of the enumeration of conjugacy classes of root data
  involutions for the given inner class, or equivalently that of twisted
  involutions in the Weyl group, which is a pure Weyl group computation.
  Fokko's original code proceeded more or less by listing each of these
  conjugacy classes, checking each new twisted involution for membership of
  each class of previously generated ones. Now we find for each conjugacy
  class of twisted involutions a canonical representative, which is relatively
  easy to compute. In this way we avoid enumeration of each conjugacy class.

  The identification of real forms can be done using the list of Cartan
  classes as well, via the unique most split Cartan class for each real form.

  The most delicate part is the "correlation" part: for each Cartan, tell
  which orbit in the corresponding fiber corresponds to which real form, the
  real forms being labelled by the orbits in the fundamental fiber. In the
  current version, the solution to this problem is cleaner then previously,
  and perfectly general: it is obtained by writing out a system of equations
  for the grading defining the real form; this system does not always have a
  unique solution, but all solutions correspond to the same real form.


******************************************************************************/

namespace atlas {

namespace complexredgp {

  void crossTransform(RootNbrList&,
		      const WeylWord&,
		      const RootSystem&);

  unsigned long makeRepresentative(const Grading&,
				   const RootNbrList&,
				   const Fiber&);

  bool checkDecomposition(const TwistedInvolution& ti,
			  const WeylWord& cross,
			  const RootNbrSet& Cayley,
			  const TwistedWeylGroup& W,
			  const RootSystem& rs);

} // |namespace complexredgp|


/*****************************************************************************

        Chapter I -- The ComplexReductiveGroup class

******************************************************************************/

namespace complexredgp {

ComplexReductiveGroup::C_info::C_info
  (const ComplexReductiveGroup& G,const TwistedInvolution twi, CartanNbr i)
  : tw(twi)
  , real_forms(G.numRealForms()), dual_real_forms(G.numDualRealForms())
  , rep(G.numRealForms()),        dual_rep(G.numDualRealForms())
  , below(i)
  , class_pt(NULL)
  , real_labels(), dual_real_labels() // these start out emtpy
  {}


/*
  Main constructor

  Constructs a |ComplexReductiveGroup| from a pre-rootdatum |rd| and a
  distinguished involution |d|, which stabilises the set of simple roots
*/
ComplexReductiveGroup::ComplexReductiveGroup
 (const PreRootDatum& prd, const WeightInvolution& tmp_d)
  : d_rootDatum(prd)
  , d_dualRootDatum(d_rootDatum,tags::DualTag())
  , d_fundamental(d_rootDatum,tmp_d) // will also be fiber of cartan(0)
  , d_dualFundamental(d_dualRootDatum,dualBasedInvolution(tmp_d,d_rootDatum))
    // dual fundamental fiber is dual fiber of most split Cartan

  , my_W(new WeylGroup(d_rootDatum.cartanMatrix()))
  , W(*my_W) // owned when this constructor is used

  , d_titsGroup(d_rootDatum,W,distinguished())
  , d_dualTitsGroup(d_dualRootDatum,W,dualDistinguished())
  , root_twist(d_rootDatum.root_permutation(simple_twist()))

  , Cartan(1,C_info(*this,TwistedInvolution(),0))
  , Cartan_poset() // poset is extended and populated below
  , d_mostSplit(numRealForms(),0) // values 0 may be increased below

  , C_orb(d_rootDatum,distinguished(),d_titsGroup)// don't store ref to |d|!
{
  construct();
}

/*
  Variant constructor, differs only by using a constructed root datum

  Constructs a |ComplexReductiveGroup| from a rootdatum |rd| and a
  distinguished involution |d|, which stabilises the set of simple roots
*/
ComplexReductiveGroup::ComplexReductiveGroup
 (const RootDatum& rd, const WeightInvolution& tmp_d)
  : d_rootDatum(rd)
  , d_dualRootDatum(d_rootDatum,tags::DualTag())
  , d_fundamental(d_rootDatum,tmp_d) // will also be fiber of cartan(0)
  , d_dualFundamental(d_dualRootDatum,dualBasedInvolution(tmp_d,d_rootDatum))
    // dual fundamental fiber is dual fiber of most split Cartan

  , my_W(new WeylGroup(d_rootDatum.cartanMatrix()))
  , W(*my_W) // owned when this constructor is used

  , d_titsGroup(d_rootDatum,W,distinguished())
  , d_dualTitsGroup(d_dualRootDatum,W,dualDistinguished())
  , root_twist(d_rootDatum.root_permutation(simple_twist()))

  , Cartan(1,C_info(*this,TwistedInvolution(),0))
  , Cartan_poset() // poset is extended and populated below
  , d_mostSplit(numRealForms(),0) // values 0 may be increased below

  , C_orb(d_rootDatum,distinguished(),d_titsGroup)// don't store ref to |d|!
{
  construct();
}

void ComplexReductiveGroup::construct()
{
  { // task 1: generate Cartan classes, fill non-dual part of |Cartan|
    const TitsCoset adj_Tg(*this);     // based adjoint Tits group
    const TitsGroup& Tg=adj_Tg.titsGroup(); // same, forgetting base

    {
      const Fiber& f=fundamental();
      const Partition& weak_real=f.weakReal();
      // fill initial |form_reps| vector with assignment from |weak_real|
      for (unsigned long i=0; i<weak_real.classCount(); ++i)
      {
	Cartan[0].real_forms.insert(i); // Cartan 0 exists at all real forms
	// setting the initial torus part for each real form is subtle.
	Cartan[0].rep[i]=              // in |adj_Tg|, a torus part is a
	  cartanclass::restrictGrading // |Grading| of simple roots, where
	  (f.compactRoots(weak_real.classRep(i)), // compact ones need bit set
	   d_rootDatum.simpleRootList()); // flipping base (noncompact) grading
      }
    }

    for (CartanNbr i=0; i<Cartan.size(); ++i) // |Cartan| grows as loop advances
    {
      Cartan_poset.new_max(Cartan[i].below); // include now completed level

#ifndef NDEBUG
      CartanNbr entry_level=Cartan.size();
#endif
      InvolutionData id =
	InvolutionData::build(d_rootDatum,d_titsGroup,Cartan[i].tw);
      RootNbrSet pos_im = id.imaginary_roots() & d_rootDatum.posRootSet();
      for (RootNbrSet::iterator it=pos_im.begin(); it(); ++it)
      {
	RootNbr alpha=*it;
	SmallBitVector alpha_bin(d_rootDatum.inSimpleRoots(alpha));

	// create a test element with null torus part
	TitsElt a(Tg,Cartan[i].tw);
	WeylWord conjugator;

	size_t j; // declare outside loop to allow inspection of final value
	while (alpha!=d_rootDatum.simpleRootNbr
				   (j=d_rootDatum.find_descent(alpha)))
	{
	  conjugator.push_back(j);
	  adj_Tg.basedTwistedConjugate(a,j);
	  d_rootDatum.simple_reflect_root(alpha,j);
	}

	bool zero_grading = adj_Tg.simple_grading(a,j); // grading of test elt

	TwistedInvolution sigma = W.prod(j,a.tw()); // "Cayley transform"
	WeylWord ww = W.word(canonicalize(sigma));

	CartanNbr ii;
	for (ii=0; ii<Cartan.size(); ++ii)
	  if (Cartan[ii].tw==sigma)
	    break; // found a previously encountered Cartan class

	if (ii==Cartan.size())
	  Cartan.push_back(C_info(*this,sigma,ii));

	Cartan[ii].below.insert(i);

	for (BitMap::iterator rfi=Cartan[i].real_forms.begin(); rfi(); ++rfi)
	{
	  RealFormNbr rf = *rfi;
	  const RankFlags in_rep = Cartan[i].rep[rf];
	  RankFlags& out_rep = Cartan[ii].rep[rf]; // to be filled in
	  tits::TorusPart tp(in_rep,alpha_bin.size());
	  if (alpha_bin.dot(tp)!=zero_grading)
	  {
	    if (not Cartan[ii].real_forms.isMember(rf))
	    { // this is the first hit of |ii| for |rf|
	      assert(ii>=entry_level); // we may populate only newborn sets
	      Cartan[ii].real_forms.insert(rf); // mark existence for |rf|

	      TitsElt x(Tg,tp,Cartan[i].tw);
	      adj_Tg.basedTwistedConjugate(x,conjugator);
	      adj_Tg.Cayley_transform(x,j);
	      adj_Tg.basedTwistedConjugate(x,ww);
	      assert(x.tw()==sigma);

	      out_rep=Tg.left_torus_part(x).data();
	      d_mostSplit[rf]=ii; // the last such assignment for |rf| sticks
	    }
	  }
	} // for (rf)
      } // for (alpha)
    } // |for (i<Cartan.size())|

    C_orb.set_size(Cartan.size());
  } // task 1 (Cartan class generation)

  { // task 2: fill remainder of all |Cartan[i]|: |dual_real_forms|, |dual_rep|
    const TitsCoset dual_adj_Tg (*this,tags::DualTag());
    const TitsGroup& dual_Tg = dual_adj_Tg.titsGroup();

    { // first initialise |Cartan.back().dual_rep|
      TwistedInvolution w0(W.longest()); // this is always a twisted inv.
      WeylWord ww = W.word(canonicalize(w0)); // but not always canonical
      assert(w0==Cartan.back().tw);

      const Fiber& f=dualFundamental();
      const Partition& weak_real=f.weakReal();
      // fill initial |form_reps| vector with assignment from |weak_real|
      for (unsigned long i=0; i<weak_real.classCount(); ++i)
      {
	// most split Cartan exists at all dual real forms
	Cartan.back().dual_real_forms.insert(i);

	RankFlags gr = // as above, torus part is obtained as a grading
	  cartanclass::restrictGrading // values at simple roots give torus part
	  (f.compactRoots(weak_real.classRep(i)), // compact ones need a set bit
	   d_rootDatum.simpleRootList()); // reproduces grading at imag. simples
	TitsElt x(dual_Tg,tits::TorusPart(gr,d_rootDatum.semisimpleRank()));
	dual_adj_Tg.basedTwistedConjugate(x,ww); // transform to canonical
	Cartan.back().dual_rep[i] = dual_Tg.left_torus_part(x).data();
      }
    }

    for (CartanNbr i=Cartan.size(); i-->0; )
    {
      InvolutionData id =
	InvolutionData::build(d_rootDatum,d_titsGroup,Cartan[i].tw);
      RootNbrSet pos_re = id.real_roots() & d_rootDatum.posRootSet();
      for (RootNbrSet::iterator it=pos_re.begin(); it(); ++it)
      {
	RootNbr alpha=*it;
	SmallBitVector alpha_bin(d_rootDatum.inSimpleCoroots(alpha));

	TwistedInvolution tw = Cartan[i].tw; // non-dual
	TwistedInvolution tw_dual = W.opposite(tw);

	// create a test element with null torus part
	TitsElt a(dual_Tg,tw_dual);
	WeylWord conjugator;

	size_t j; // declare outside loop to allow inspection of final value
	while (alpha!=d_rootDatum.simpleRootNbr
				   (j=d_rootDatum.find_descent(alpha)))
	{
	  conjugator.push_back(j);
	  dual_adj_Tg.basedTwistedConjugate(a,j);
	  twistedWeylGroup().twistedConjugate(tw,j);
	  d_rootDatum.simple_reflect_root(alpha,j);
	}

	bool zero_grading = dual_adj_Tg.simple_grading(a,j);

	assert(tw==W.opposite(a.tw())); // coherence with dual group

	W.leftMult(tw,j); // "Cayley transform"
	WeylWord ww=W.word(canonicalize(tw)); // in non-dual setting

	CartanNbr ii;

	for (ii=i; ii-->0; )
	  if (Cartan[ii].tw==tw)
	    break; // found a previously encountered Cartan class (must happen)
	assert(ii<Cartan.size() and Cartan[i].below.isMember(ii));

	for (BitMap::iterator
	       drfi=Cartan[i].dual_real_forms.begin(); drfi(); ++drfi)
	{
	  RealFormNbr drf = *drfi;
	  const RankFlags in_rep = Cartan[i].dual_rep[drf];
	  RankFlags& out_rep = Cartan[ii].dual_rep[drf];
	  tits::TorusPart tp(in_rep,alpha_bin.size());
	  if (alpha_bin.dot(tp)!=zero_grading)
	  {
	    if (not Cartan[ii].dual_real_forms.isMember(drf))
	    { // this is the first hit of |ii| for |rf|
	      Cartan[ii].dual_real_forms.insert(drf);

	      TitsElt x(dual_Tg,tp,tw_dual);
	      dual_adj_Tg.basedTwistedConjugate(x,conjugator);
	      dual_adj_Tg.Cayley_transform(x,j);
	      dual_adj_Tg.basedTwistedConjugate(x,ww);
	      assert(x.tw()==W.opposite(tw));

	      out_rep=dual_Tg.left_torus_part(x).data();
	    }
	  }
	} // for (rf)
      } // for (alpha)
    } // |for (i=Cartan.size()-->0)|

  }
}

/*!
  \brief constructs the complex reductive group dual to G.
*/
ComplexReductiveGroup::ComplexReductiveGroup(const ComplexReductiveGroup& G,
					     tags::DualTag)
  : d_rootDatum(G.d_dualRootDatum)
  , d_dualRootDatum(G.d_rootDatum)
  , d_fundamental(G.d_dualFundamental)
  , d_dualFundamental(G.d_fundamental)

  , my_W(NULL), W(G.W) // not owned here, we depend on existence of |G|

  , d_titsGroup(G.d_dualTitsGroup,W)
  , d_dualTitsGroup(G.d_titsGroup,W)

  , root_twist(d_rootDatum.root_permutation(simple_twist()))

  , Cartan() // filled below
  , Cartan_poset(G.Cartan_poset,tags::DualTag())
  , d_mostSplit(numRealForms(),0) // values 0 may be increased below

  , C_orb(d_rootDatum,d_fundamental.involution(),d_titsGroup)
{
  Cartan.reserve(G.Cartan.size());
  C_orb.set_size(G.Cartan.size());

  for (CartanNbr i=G.Cartan.size(); i-->0; )
  {
    const C_info& src = G.Cartan[i];
    Cartan.push_back(C_info(*this,W.opposite(src.tw),Cartan.size()));
    C_info& dst = Cartan.back();

    const TitsCoset adj_Tg(*this);     // based adjoint Tits group
    const TitsGroup& Tg=adj_Tg.titsGroup(); // same, forgetting base
    const TitsCoset dual_adj_Tg (*this,tags::DualTag());
    const TitsGroup& dual_Tg = dual_adj_Tg.titsGroup();

    const TwistedInvolution tw_org = dst.tw;
    const TwistedInvolution dual_tw_org = src.tw;

    WeylWord conjugator = W.word(canonicalize(dst.tw));

    dst.real_forms = src.dual_real_forms;
    dst.dual_real_forms = src.real_forms;
    dst.rep = src.dual_rep; // these are torus parts at |tw_org|, and this
    dst.dual_rep = src.rep; // assignment is mainly to set their size in |dst|

    for (BitMap::iterator it=dst.real_forms.begin(); it(); ++it)
    {
      TitsElt x(Tg,
		      tits::TorusPart(dst.rep[*it],semisimpleRank()),
		      tw_org);
      adj_Tg.basedTwistedConjugate(x,conjugator);
      assert(x.tw()==dst.tw);
      dst.rep[*it] =Tg.left_torus_part(x).data();
      d_mostSplit[*it]=Cartan.size()-1; // last occurrence of |*it| will stick
    }
    for (BitMap::iterator it=dst.dual_real_forms.begin(); it(); ++it)
    {
      TitsElt y(dual_Tg,
		      tits::TorusPart(dst.dual_rep[*it],semisimpleRank()),
		      dual_tw_org);
      dual_adj_Tg.basedTwistedConjugate(y,conjugator);
      assert(y.tw()==W.opposite(dst.tw));
      dst.dual_rep[*it] = dual_Tg.left_torus_part(y).data();
    }

    for (CartanNbr j=i+1; j<G.Cartan.size(); ++j)
      if (G.Cartan[j].below.isMember(i))
	dst.below.insert(G.Cartan.size()-1-j);
  }

}


/*!
  Free Weyl group is owned, and any non-NULL |CartanClass| pointers.
*/
ComplexReductiveGroup::~ComplexReductiveGroup()
{
  for (CartanNbr i = 0; i<numCartanClasses(); ++i)
    delete Cartan[i].class_pt;
  delete my_W;
}

/********* copy, assignment and swap *****************************************/

// This should remain empty!

} // namespace complexredgp

/*****************************************************************************

        Chapter II --- private (auxiliary) methods for ComplexReductiveGroup

******************************************************************************/

namespace complexredgp {

/******** private accessors **************************************************/

/*!
  \brief Returns |tw| composed to the left with the reflection $s_\alpha$
  corresponding to root $\alpha$

  This is a twisted involution if $s_\alpha$ twisted-commutes with |tw|;
  in practice root $\alpha$ will in fact be imaginary for |tw|
*/
TwistedInvolution
ComplexReductiveGroup::reflection(RootNbr alpha,
				  const TwistedInvolution& tw) const
{
  WeylWord rw=rootDatum().reflectionWord(alpha);

  TwistedInvolution result=tw;
  for (size_t i=rw.size(); i-->0; ) // left multiply |tw| by |rw|
    W.leftMult(result,rw[i]);

  return result;
}


/******** manipulators *******************************************************/



void ComplexReductiveGroup::add_Cartan(CartanNbr cn)
{
  Cartan[cn].class_pt =
    new CartanClass(rootDatum(),dualRootDatum(),
				 involutionMatrix(Cartan[cn].tw));
  map_real_forms(cn);      // used to be |correlateForms(cn);|
  map_dual_real_forms(cn); // used to be |correlateDualForms(cn);|
}

void ComplexReductiveGroup::map_real_forms(CartanNbr cn)
{
  TitsCoset adj_Tg(*this);
  const Fiber& f = Cartan[cn].class_pt->fiber();
  const Partition& weak_real = f.weakReal();
  const TwistedInvolution& tw = Cartan[cn].tw;
  const RootNbrList& sim = f.simpleImaginary();

  Cartan[cn].real_labels.resize(weak_real.classCount());

  tits::TorusPart base = sample_torus_part(cn,quasisplit());
  TitsElt a(adj_Tg.titsGroup(),base,tw);
  Grading ref_gr; // reference grading for quasisplit form
  for (size_t i=0; i<sim.size(); ++i)
    ref_gr.set(i,adj_Tg.grading(a,sim[i]));

  cartanclass::AdjointFiberElt rep = f.gradingRep(ref_gr);

  // now lift |rep| to a torus part and subtract from |base|
  SmallBitVector v(RankFlags(rep),
				 f.adjointFiberRank());
  base -= f.adjointFiberGroup().fromBasis(v);

  for (BitMap::iterator
	 rfi=Cartan[cn].real_forms.begin(); rfi(); ++rfi)
  {
    RealFormNbr rf = *rfi;
    tits::TorusPart tp = sample_torus_part(cn,rf);
    cartanclass::AdjointFiberElt rep =
      f.adjointFiberGroup().toBasis(tp-=base).data().to_ulong();
    Cartan[cn].real_labels[weak_real.class_of(rep)]=rf;
  }
  assert(Cartan[cn].real_labels[0]==quasisplit());
  Cartan[cn].rep[0] = base.data(); // change representative to remember base
}

void ComplexReductiveGroup::map_dual_real_forms(CartanNbr cn)
{
  TitsCoset dual_adj_Tg(*this,tags::DualTag());
  const Fiber& dual_f = Cartan[cn].class_pt->dualFiber();
  const Partition& dual_weak_real = dual_f.weakReal();
  const TwistedInvolution dual_tw =W.opposite(Cartan[cn].tw);
  const RootNbrList& sre = dual_f.simpleImaginary(); // simple real

  Cartan[cn].dual_real_labels.resize(dual_weak_real.classCount());
  assert(dual_weak_real.classCount()==Cartan[cn].dual_real_forms.size());

  tits::TorusPart dual_base = dual_sample_torus_part(cn,0);
  TitsElt dual_a(dual_adj_Tg.titsGroup(),dual_base,dual_tw);
  Grading dual_ref_gr; // reference for dual quasisplit form
  for (size_t i=0; i<sre.size(); ++i)
    dual_ref_gr.set(i,dual_adj_Tg.grading(dual_a,sre[i]));

  cartanclass::AdjointFiberElt dual_rep = dual_f.gradingRep(dual_ref_gr);

  // now lift |rep| to a torus part and subtract from |base|
  SmallBitVector v(RankFlags(dual_rep),
				 dual_f.adjointFiberRank());
  dual_base -= dual_f.adjointFiberGroup().fromBasis(v);

  for (BitMap::iterator
	 drfi=Cartan[cn].dual_real_forms.begin(); drfi(); ++drfi)
  {
    RealFormNbr drf = *drfi;
    tits::TorusPart tp = dual_sample_torus_part(cn,drf);
    cartanclass::AdjointFiberElt rep =
      dual_f.adjointFiberGroup().toBasis(tp-=dual_base).data().to_ulong();
    Cartan[cn].dual_real_labels[dual_weak_real.class_of(rep)]=drf;
  }
  assert(Cartan[cn].dual_real_labels[0]==0);
  Cartan[cn].dual_rep[0] = dual_base.data();
}

#if 0 // functions below are no longer used
/*!
  \brief Adds a new real form label list to d_realFormLabels.

  This is called when a new |CartanClass| has just been constructed at index
  |cn|. Then this function finds the labels corresponding to the real forms
  for which this Cartan is defined (the labelling of real forms being defined
  by the adjoint orbit picture in the fundamental fiber.)

  Algorithm: the gradings of the imaginary root system corresponding to the
  various real forms for which the new Cartan is defined are known. We find
  a cross-action followed by a composite Cayley transform, taking the
  fundamental Cartan to the new one. Then for each real form, we take a
  representative grading, and compute a grading for the fundamental Cartan
  transforming to it. This amounts to solving a system of linear equations
  mod 2.
*/
void ComplexReductiveGroup::correlateForms(CartanNbr cn)
{
  const RootSystem& rs = rootDatum();
  const TwistedWeylGroup& tW = twistedWeylGroup();
  const Fiber& fundf = d_fundamental;
  const Fiber& f = cartan(cn).fiber(); // fiber of this Cartan

  const TwistedInvolution& ti = twistedInvolution(cn);

  // find cayley part and cross part
  RootNbrSet so;
  WeylWord ww;
  Cayley_and_cross_part(so,ww,ti,rs,tW);

  assert(checkDecomposition(ti,ww,so,tW,rs));

  const Partition& pi = f.weakReal();
  RealFormNbrList rfl(f.numRealForms());

  // transform gradings and correlate forms
  for (size_t j = 0; j < rfl.size(); ++j)
  {
    unsigned long y = pi.classRep(j); //  orbit |j| has representative |y|
    Grading gr=f.grading(y); // and |gr| is it's grading
    RootNbrList rl = f.simpleImaginary(); // of the roots in |rl|

    gradings::transform_grading(gr,rl,so,rs); // grade those roots at |fundf|
    for (size_t i = 0; i < so.size(); ++i)
      gr.set(rl.size()+i);        // make grading noncompact for roots in |so|
    std::copy(so.begin(),so.end(),back_inserter(rl)); // extend |rl| with |so|
    crossTransform(rl,ww,rs);  // apply cross part of |ti| to roots in |rl|

    /* now |gr| grades the roots in |rl|,
       which are imaginary for the fundamental fiber |fundf| */
    for (size_t i = 0; i < rl.size(); ++i)
      assert(fundf.imaginaryRootSet().isMember(rl[i]));

    unsigned long x = makeRepresentative(gr,rl,fundf);
    RealFormNbr rf = fundf.weakReal()(x); // look up representative
    rfl[j] = rf;
  }

  Cartan[cn].real_labels = rfl;
  assert(rfl[0]==quasisplit()); // adjoint base grading is always quasisplit
}

void ComplexReductiveGroup::correlateDualForms(CartanNbr cn)
{
  const RootSystem& rs = dualRootDatum();
  const TwistedWeylGroup& tW = dualTwistedWeylGroup();
  const Fiber& fundf = d_dualFundamental;
  const Fiber& f = cartan(cn).dualFiber();

  TwistedInvolution ti = dualTwistedInvolution(cn);


  // find cayley part and cross part
  RootNbrSet so;
  WeylWord ww;
  Cayley_and_cross_part(so,ww,ti,rs,tW); // computation is in dual setting!

  assert(checkDecomposition(ti,ww,so,tW,rs));

  const Partition& pi = f.weakReal();
  RealFormNbrList rfl(f.numRealForms());

  // transform gradings and correlate forms
  for (size_t j=0; j<rfl.size(); ++j)
  {
    unsigned long y = pi.classRep(j);
    Grading gr=f.grading(y);
    RootNbrList rl = f.simpleImaginary();
    gradings::transform_grading(gr,rl,so,rs);
    for (size_t i=0; i<so.size(); ++i)
      gr.set(rl.size()+i);
    copy(so.begin(),so.end(),back_inserter(rl));
    crossTransform(rl,ww,rs);

// begin testing
    /* now |gr| grades the roots in |rl|,
       which are imaginary for the dual fundamental fiber |fundf| */
    for (size_t i=0; i<rl.size(); ++i)
      assert(fundf.imaginaryRootSet().isMember(rl[i]));
// end testing

    unsigned long x = makeRepresentative(gr,rl,fundf);
    RealFormNbr rf = fundf.weakReal()(x);
    rfl[j] = rf;
  }

  Cartan[cn].dual_real_labels = rfl;
  assert(rfl[0]==0); // adjoint base grading is always quasisplit
}

/*!\brief
  Checks whether |ti| decomposes as the composition of the cross-action
  defined by |cross| followed by the Cayley transform defined by |Cayley|.
*/
bool checkDecomposition(const TwistedInvolution& ti,
			const WeylWord& cross,
			const RootNbrSet& Cayley,
			const TwistedWeylGroup& W,
			const RootSystem& rs)
{
  TwistedInvolution tw;

  // cross action part
  for (size_t i=0; i<cross.size(); ++i)
    W.twistedConjugate(tw,cross[i]);

  // cayley transform part
  for (RootNbrSet::iterator it=Cayley.begin(); it(); ++it)
  {
    InvolutionData id(rs,W.simple_images(rs,tw));
    assert(id.imaginary_roots().isMember(*it));
    W.leftMult(tw,rs.reflectionWord(*it));
  }

  return tw == ti;
}

#endif // remainder is compiled again

/*!
  \brief Returns the size of the fiber orbits corresponding to the strong
  real forms lying over (weak) real form \#rf, in cartan \#cn.

  Precondition: Real form \#rf is defined for cartan \#cn.
*/
unsigned long
ComplexReductiveGroup::fiberSize(RealFormNbr rf, CartanNbr cn)
{
  cartanclass::adjoint_fiber_orbit wrf = real_form_part(rf,cn);
  // |wrf| indexes a $W_{im}$ orbit on |cartan(cn).fiber().adjointFiberGroup()|

  const Fiber& f = cartan(cn).fiber();
  const cartanclass::StrongRealFormRep& srf = f.strongRealForm(wrf);

  assert(srf.second==f.central_square_class(wrf));

  // get partition of the fiber group according to action for square class
  const Partition& pi =f.fiber_partition(srf.second);
  // |pi| is an (unnormalized) partition of |cartan(cn).fiber().fiberGroup()|

  return pi.classSize(srf.first); // return size of orbit number |srf.first|
}

/*!
  \brief Returns the size of the dual fiber orbits corresponding to
  the dual strong real forms lying over dual real form \#rf, in Cartan
  \#cn.

  Precondition: real form \#rf is defined for cartan \#cn.
*/

unsigned long
ComplexReductiveGroup::dualFiberSize(RealFormNbr rf, CartanNbr cn)
{
  cartanclass::adjoint_fiber_orbit wrf=dual_real_form_part(rf,cn);

  const Fiber& df = cartan(cn).dualFiber();
  const cartanclass::StrongRealFormRep& srf = df.strongRealForm(wrf);

  assert(srf.second==df.central_square_class(wrf));

  const Partition& pi = df.fiber_partition(srf.second);
  return pi.classSize(srf.first);
}



/*****************************************************************************

        Chapter III --- public accessor methods for ComplexReductiveGroup

******************************************************************************/


/******** accessors **********************************************************/

BitMap
ComplexReductiveGroup::Cartan_set(RealFormNbr rf) const
{
  BitMap support(Cartan.size());
  for (CartanNbr i=0; i<Cartan.size(); ++i)
    if (Cartan[i].real_forms.isMember(rf))
      support.insert(i);

  return support;
}

BitMap
ComplexReductiveGroup::dual_Cartan_set(RealFormNbr drf) const
{
  BitMap support(Cartan.size());
  for (CartanNbr i=0; i<Cartan.size(); ++i)
    if (Cartan[i].dual_real_forms.isMember(drf))
      support.insert(i);

  return support;
}

/*!
  \brief Returns the total number of involutions (generating Cartans as needed)
*/
InvolutionNbr ComplexReductiveGroup::numInvolutions()
{
  InvolutionNbr count = 0;

  for (CartanNbr cn=0; cn<numCartanClasses(); ++cn)
    count += cartan(cn).orbitSize();

  return count;
}

/*!
  \brief Returns the total number of involutions corresponding to the
  indicated set of Cartans.
*/
InvolutionNbr ComplexReductiveGroup::numInvolutions
  (const BitMap& Cartan_classes)
{
  InvolutionNbr count = 0;

  for (BitMap::iterator it=Cartan_classes.begin(); it(); ++it)
    count += cartan(*it).orbitSize();

  return count;
}


WeightInvolution
ComplexReductiveGroup::involutionMatrix(const TwistedInvolution& tw)
  const
{
  return rootDatum().matrix(weylGroup().word(tw.w())) * distinguished();
}

/*!
  \brief Flags in rs the set of noncompact positive roots for Cartan \#j.
*/
RootNbrSet ComplexReductiveGroup::noncompactPosRootSet
  (RealFormNbr rf, size_t j)
{
  const Fiber& f = cartan(j).fiber();
  unsigned long x = representative(rf,j); // real form orbit-representative

  return f.noncompactRoots(x) & rootDatum().posRootSet();
}

/*!
\brief Sum of the real roots.
*/
Weight
  ComplexReductiveGroup::posRealRootSum(const TwistedInvolution& tw)
  const
{
  return rootDatum().twoRho(involution_data(tw).real_roots());
}

  /*!
\brief Sum of the imaginary roots.
  */
Weight
  ComplexReductiveGroup::posImaginaryRootSum(const TwistedInvolution& tw)
  const
{
  return rootDatum().twoRho(involution_data(tw).imaginary_roots());
}

/*! \brief Make |sigma| canonical and return Weyl group |w| element that
    twisted conjugates the canonical representative back to original |sigma|.

    We find conjugating generators starting at the original `|sigma|' end, so
    these form the letters of |w| from left (last applied) to right (first).
*/
WeylElt // return value is conjugating element
ComplexReductiveGroup::canonicalize
  (TwistedInvolution &sigma, // element to modify
   RankFlags gens) // subset of generators
  const
{
  return complexredgp::canonicalize(sigma,rootDatum(),twistedWeylGroup(),gens);
}

//!\brief find number of Cartan class containing twisted involution |sigma|
CartanNbr ComplexReductiveGroup::class_number(TwistedInvolution sigma) const
{
  canonicalize(sigma);
  for (CartanNbr i=0; i<Cartan.size(); ++i)
    if (sigma==Cartan[i].tw)
      return i;

  assert(false); // all canonical twisted involutions should occur in |Cartan|
  return ~0;
}


/*!
  \brief Returns the cardinality of the subset of \f$K\backslash G/B\f$
   associated to |rf| whose twisted involutions belong to |Cartan_classes|.
*/
unsigned long
ComplexReductiveGroup::KGB_size(RealFormNbr rf,
				const BitMap& Cartan_classes)
{
  unsigned long result=0;
  for (BitMap::iterator it = Cartan_classes.begin(); it(); ++it)
    result +=  cartan(*it).orbitSize() * fiberSize(rf,*it);

  return result;

}

/*! \brief
  Returns the cardinality of the union of sets \f$K\backslash G/B\f$ for this
  inner class.

  (Here each real form appears as often as there are strong real forms for it
  in its square class)
*/
unsigned long
ComplexReductiveGroup::global_KGB_size()
{
  unsigned long result=0;
  for (CartanNbr cn=0; cn<numCartanClasses(); ++cn)
  {
    const CartanClass& cc = cartan(cn);
    result += cc.orbitSize() * cc.numRealFormClasses() * cc.fiber().fiberSize();
  }
  return result;

}

unsigned long
ComplexReductiveGroup::block_size(RealFormNbr rf,
				  RealFormNbr drf,
				  const BitMap& Cartan_classes)
{
  unsigned long result=0;
  for (BitMap::iterator it = Cartan_classes.begin(); it(); ++it)
  {
    unsigned long cn=*it;
    result +=
      cartan(cn).orbitSize() * fiberSize(rf,cn) * dualFiberSize(drf,cn);
  }

  return result;

}

/*!
\brief Modify |v| through through involution associated to |tw|
*/
void ComplexReductiveGroup::twisted_act
  (const TwistedInvolution& tw,Weight& v) const
{
  distinguished().apply_to(v);
  weylGroup().act(rootDatum(),tw.w(),v);
}



} // namespace complexredgp

/*****************************************************************************

        Chapter IV -- Functions declared in complexredgp.h

******************************************************************************/

namespace complexredgp {

WeylElt canonicalize // return value is conjugating element
  (TwistedInvolution& sigma,
   const RootDatum& rd,
   const TwistedWeylGroup& W,
   RankFlags gens)
{
  const RootNbrList s_image=W.simple_images(rd,sigma);
  InvolutionData id(rd,s_image);

  Weight rrs=rd.twoRho(id.real_roots());
  Weight irs=rd.twoRho(id.imaginary_roots());

/* the code below uses the following fact: if $S$ is a root subsystem of |rd|,
   and $\alpha$ a simple root that does not lie in $S$, then the sum of
   positive roots of $s_\alpha(S)$ is the image by $s_\alpha$ of the sum of
   positive roots of $S$. The reason is that the action of $s_\alpha$ almost
   preseves the notion of positivity; it only fails for the roots $\pm\alpha$,
   which do not occur in $S$ or in $s_\alpha(S)$. The code only applies
   $s_\alpha$ when the sum of positive of roots of $S$ is strictly
   anti-dominant for $\alpha$, and $S$ is either the system of real or
   imaginary roots; then $\alpha$ is a complex root, and in particular
   $\alpha$ does not lie in $S$.
 */

  WeylElt w; // initialized to identity; this will be the result

  { // first phase: make |rrs| dominant for all complex simple roots in |gens|
    // and make |irs| dominant for all such roots that are orthogonal to |rrs|
    RankFlags::iterator it; // allow inspection of final value
    do
      for (it=gens.begin(); it(); ++it)
      {
	size_t i=*it;
	LatticeCoeff c=rrs.dot(rd.simpleCoroot(i));
	if (c<0 or (c==0 and irs.dot(rd.simpleCoroot(i))<0))
	{
	  rd.reflect(rrs,rd.simpleRootNbr(i));   // apply $s_i$ to re-root sum
	  rd.reflect(irs,rd.simpleRootNbr(i));   // apply $s_i$ to im-root sum
	  W.twistedConjugate(sigma,i); // adjust |sigma| accordingly
	  W.mult(w,i);                 // and add generator to |w|
	  break;     // after this change, continue the |do|-|while| loop
	}
      }
    while (it()); // i.e., until no change occurs any more
  }

/*
  Now that |rrs| and |irs| are dominant vectors, the simple coroots have non
  negative values on them. Any positive coroot is the sum of a multiset of
  simple coroots, and if that coroot is orthogonal to |rrs| and |irs|, then
  its constituents must be so as well, since there can be no cancellation in
  its evaluations on |rrs| and |irs|. Therefore the root subsystem orhogonal
  to |rrs| and |irs| is generated by a subset of the simple roots.
 */

  // clear those simple roots in |gens| not orthogonal to |irs|
  for (RankFlags::iterator it=gens.begin(); it(); ++it)
    if (rrs.dot(rd.simpleCoroot(*it))>0 or irs.dot(rd.simpleCoroot(*it))>0)
      gens.reset(*it);


/* Now ensure that the involution |theta| associated to the twisted involution
   |sigma| fixes the dominant chamber for the root subsystem now flagged in
   |gens|, which we shall call the complex root subsystem. Since |theta|
   stablises this subsytem globally, this means it must be made to permute its
   positive and negative roots separately. We repeatedly inspect the simple
   roots of this subsystem, searching for some $\alpha_i$ that maps to a
   negative root; each time one is found, we twisted-conjugate |sigma| by $i$,
   which improves the situation (one can think of the twisted-conjugation as
   changing the positivity status of (only) $\alpha_i$, although the new
   statuses actually apply to the $\alpha_i$-reflected images of the roots).
   Eventually all positive roots in the subset will map to positive roots.
*/
  {
    RankFlags::iterator it;
    do
      for (it=gens.begin(); it(); ++it)
      {
	size_t i=*it;
	RootNbr beta= // image of |rd.simpleRootNbr(i)| by $\theta$
	  rd.permuted_root(W.word(sigma.w()),rd.simpleRootNbr(W.twisted(i)));
	if (not rd.isPosRoot(beta))
	{
	  W.twistedConjugate(sigma,i); // adjust |sigma|
	  W.mult(w,i);                 // and add generator to |w|
	  break;                       // and continue |do|-|while| loop
	}
      }
    while (it()); // i.e., while |for| loop was interrupted
  }

  return  w; // but the main result is the modfied value left in |sigma|
}

/*!
  \brief Puts into |so| the composite Cayley transform, and into |cross| the
   cross action corresponding to the twisted involution |ti|.

  Explanation: to each root datum involution $q$, we may associate a
  transformation from the fundamental involution to $q$ that factors as the
  composition of a cross action (conjugation by the inverse of an element
  |cross| of |W|), followed by a composite Cayley transform (composition at
  left or right with the product of (commuting) reflections for the roots of a
  strongly orthogonal set |so| of imaginary roots). This function computes
  |cross| and |so|, where $q$ is given by the twisted involution |ti|. Neither
  of the two parts of this decomposition are unique; for instance in the equal
  rank case the initial conjugation is entirely without effect.

  Note that conjugation is by the inverse of |cross| only because conjugation
  uses the letters of a Weyl word successively from right to left, whereas we
  collect the letters of |cross| from left to right as we go from the
  fundamental involution (the trivial twisted involution representing) back to
  $q$. While cross actions and Cayley transforms are found in an interleaved
  fashion, we push the latter systematically to the end (left) in the result;
  this means the corresponding roots are to be reflected by the cross actions
  that were found later, which cross actions themselves remain unchanged.
*/
void Cayley_and_cross_part(RootNbrSet& Cayley,
			   WeylWord& cross,
			   const TwistedInvolution& ti,
			   const RootSystem& rs,
			   const TwistedWeylGroup& W)
{
  weyl::InvolutionWord dec=W.involution_expr(ti);
  TwistedInvolution tw; // to reconstruct |ti| as a check

  RootNbrList so; // values for Cayley; |RootList| is preferable here
  cross.clear(); so.reserve(rs.rank());

  for (size_t j=dec.size(); j-->0; )
    if (dec[j]>=0) // Cayley transform by simple root
    {
      weyl::Generator s=dec[j];
      so.push_back(rs.simpleRootNbr(s));
      W.leftMult(tw,s);
    }
    else // cross action by simple root
    {
      weyl::Generator s=~dec[j];
      cross.push_back(s); // record cross action
      W.twistedConjugate(tw,s); // and twisted-conjugate |tw|
      // and conjugate roots in |so|:
      for (size_t i=0; i<so.size(); ++i)
	rs.simple_reflect_root(so[i],s); // replace root by reflection image
    }

  assert(tw==ti);

  Cayley.set_capacity(rs.numRoots());
  Cayley.insert(so.begin(),so.end());
  Cayley = rs.long_orthogonalize(Cayley);
}

} // namespace complexredgp


/*****************************************************************************

        Chapter V -- Local Functions

******************************************************************************/

namespace complexredgp {

/*!
  \brief Cross-transforms the roots in |rl| according to |ww|.

  NOTE: the cross-transformations of |ww| are done in right to left order.
*/
void crossTransform(RootNbrList& rl,
		    const WeylWord& ww,
		    const RootSystem& rs)
{
  for (size_t i = ww.size(); i-->0;)
    rs.simple_root_permutation(ww[i]).left_mult(rl);
}

/*!
  \brief Returns an element |x| (interpreted as element of the adjoint fiber
  of |fundf|) such that it grades the elements in |rl| according to |gr|.

  The successive bits of |gr| give the desired grading of the successive roots
  in |rl|. At least one solution for |x| should be known to exist. The roots
  in |rl| should probably be linearly independent, since linearly dependent
  roots would only make the existence of a solution less likely.
*/
unsigned long makeRepresentative(const Grading& gr,
				 const RootNbrList& rl,
				 const Fiber& fundf)
{
  RootNbrSet brs =
    fundf.noncompactRoots(0); // noncompact roots for the base grading
  Grading bgr =
    cartanclass::restrictGrading(brs,rl); // view as grading of roots of |rl|
  SmallBitVector bc(bgr,rl.size()); // transform to binary vector

  // make right hand side
  SmallBitVector rhs(gr,rl.size()); // view |gr| as binary vector (same length)
  rhs += bc;                        // and add the one for the base grading

  // make grading shifts
  SmallBitVectorList cl(fundf.adjointFiberRank(),bc);
  for (unsigned int i = 0; i < cl.size(); ++i)
  {
    Grading gr1 =
      cartanclass::restrictGrading(fundf.noncompactRoots(1 << i),rl);
    cl[i] += // cl[i] is shift for vector e[i]
      SmallBitVector(gr1,rl.size());
  }

  // set up equations
  RankFlags x;
  bitvector::firstSolution(x,cl,rhs);

  return x.to_ulong();
}



} // |namespace complexredgp|

} // |namespace atlas|
