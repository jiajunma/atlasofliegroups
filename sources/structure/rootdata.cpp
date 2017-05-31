/*
  This is rootdata.cpp.

  Copyright (C) 2004,2005 Fokko du Cloux
  Copyright (C) 2006--2016 Marc van Leeuwen
  part of the Atlas of Lie Groups and Representations

  For license information see the LICENSE file
*/

/*		     Implementation of the RootDatum class.

  What we call a root datum in this program is what is usually called
  a based root datum.

  The root datum defines the complex reductive group entirely.

  Another non-trivial issue is how to get a group interactively from the
  user. Actually this is perhaps a bit overstated here; in fact, when the
  program functions as a library, the interaction with the user will be
  relegated to some higher-up interface; in practice it is likely that
  the data will usually be read from a file. The main issue is to define
  the character lattice of the torus.

  In the interactive approach, we start from the abstract real lie
  type, and (implicitly) associate to it the direct product of the
  simply connected semisimple factor, and the torus as it is
  given. The user is presented with the torsion subgroup Z_tor of the
  center, written as a factor of Q/Z for each torus factor, and a
  finite abelian group for each simple factor.  The user must then
  choose generators of a finite subgroup of Z_tor; this subgroup
  corresponds to a sublattice of finite index in the character
  lattice, containing the root lattice. From this sublattice the
  actual root datum is constructed.
*/

#include "rootdata.h"

#include <cassert>
#include <set>

#include "arithmetic.h"
#include "lattice.h"
#include "bitmap.h"  // for root sets
#include "matreduc.h"
#include "ratvec.h"

#include "lietype.h"	// value returned from |LieType| method, |ext_gen|

#include "dynkin.h"
#include "weyl.h" // for class |Twist|
#include "prerootdata.h"

// extra defs for windows compilation -spc
#ifdef WIN32
#include <iterator>
#endif

/*****************************************************************************

  This module contains the implementation of the RootDatum class. What we
  call a root datum in this program is what is usually called a based root
  datum.

  The root datum defines the complex reductive group entirely.

  Another non-trivial issue is how to get a group interactively from the
  user. Actually this is perhaps a bit overstated here; in fact, when the
  program functions as a library, the interaction with the user will be
  relegated to some higher-up interface; in practice it is likely that
  the data will usually be read from a file. The main issue is to define
  the character lattice of the torus.

  In the interactive approach, we start from the abstract real lie
  type, and (implicitly) associate to it the direct product of the
  simply connected semisimple factor, and the torus as it is
  given. The user is presented with the torsion subgroup Z_tor of the
  center, written as a factor of Q/Z for each torus factor, and a
  finite abelian group for each simple factor.  The user must then
  choose generators of a finite subgroup of Z_tor; this subgroup
  corresponds to a sublattice of finite index in the character
  lattice, containing the root lattice. From this sublattice the
  actual root datum is constructed.

******************************************************************************/

namespace atlas {

namespace rootdata {


/*****************************************************************************

        Chapter I -- The RootDatum class

  The root datum class will be one of the central classes in the program.

  A based root datum is a quadruple (X,X^,D,D^), where X and X^ are lattices
  dual to each other, and D is a basis for a root system in X, D^ the basis
  of the dual root system (more precisely, D and D^ will be bases
  for root systems in the sub-lattices they generate.)

  We always X and X^ to be $\Z^n$, so specifications suffice of the basis D
  of X constisting of simple roots, and the basis D^ of X^ of simple coroots.

******************************************************************************/

struct RootSystem::root_compare
{
  root_compare() {}
  bool operator()(const Byte_vector& alpha, const Byte_vector& beta)
  {
    int d;
    for (size_t i=alpha.size(); i-->0;)
      if ((d=alpha[i]-beta[i])!=0 )
	return d<0;
    return false; // equal, therefore not less
  }
};

RootSystem::RootSystem(const int_Matrix& Cartan_matrix)
  : rk(Cartan_matrix.numRows())
  , Cmat(rk*rk) // filled below
  , ri()
  , two_rho_in_simple_roots(rk,0)
  , root_perm()
{
  if (rk==0)
    return; // avoid problems in trivial case

  std::vector<Byte_vector> simple_root(rk,Byte_vector(rk));
  std::vector<Byte_vector> simple_coroot(rk,Byte_vector(rk));

  typedef std::set<Byte_vector,root_compare> RootVecSet;
  std::vector<RootVecSet> roots_of_length
    (4*rk); // more than enough if |rk>0|; $E_8$ needs size 31

  for (size_t i=0; i<rk; ++i)
  {
    Byte_vector e_i(rk,0);
    e_i[i]=1; // set to standard basis for simple roots
    roots_of_length[1].insert(e_i);
    for (size_t j=0; j<rk; ++j)
      Cartan_entry(i,j) = simple_root[i][j] = simple_coroot[j][i] =
	Cartan_matrix(i,j);
  }

  // construct positive root list, simple reflection links, and descent sets

  std::vector<RootNbrList> link; // size |numPosRoots*rank|
  RootNbrList first_l(1,0); // where level |l| starts; level 0 is empty
  for (size_t l=1; not roots_of_length[l].empty(); ++l)// empty level means end
  {
    first_l.push_back(ri.size()); // set |first_l[l]| to next root to be added
    for (RootVecSet::iterator
	   it=roots_of_length[l].begin(); it!=roots_of_length[l].end(); ++it)
    {
      const Byte_vector& alpha = *it;
      for (size_t i=0; i<rk; ++i)
	two_rho_in_simple_roots[i]+=alpha[i]; // sum positive root expressions

      const RootNbr cur = ri.size();
      ri.push_back(root_info(alpha)); // add new positive root to the list
      link.push_back(RootNbrList(rk,RootNbr(~0))); // all links start undefined

      byte c;
      for (size_t i=0; i<rk; ++i)
	if ((c=alpha.dot(simple_coroot[i]))==0) // orthogonal
	  link[cur][i]=cur; // point to root itself
	else
	{
	  Byte_vector beta=alpha; // make a copy
	  beta[i]-=c; // increase coefficient; add |-c| times |alpha[i]|
	  if (c>0) // positive scalar product means $i$ gives a \emph{descent}
	  {
	    ri[cur].descents.set(i);
	    if (l==1)
	    {
	      assert(i==cur); // a simple root is its own unique descent
	      link[cur][i]=cur; // but reflection is actually minus itself
	    }
	    else
	      for (RootNbr j=first_l[l-c]; j<first_l[l-c+1]; ++j) // look down
		if (root(j)==beta)
		{
		  link[cur][i]=j; // link current root downward to root |j|
		  link[j][i]=cur; // and install reciprocal link at |j|
		  break;
		}
	    assert(link[cur][i]!=RootNbr(~0)); // some value must have been set
	  }
	  else // |c<0| so, reflection adding |-c| times $\alpha_j$, goes up
	  {
	    ri[cur].ascents.set(i);
	    roots_of_length[l-c].insert(beta); // create root at proper length
	  }
	}
    }
    roots_of_length[l].clear(); // no longer needed
  }

  size_t npos = ri.size(); // number of positive roots

  // simple coroots in themselves are just standard basis, like simple roots
  for (size_t i=0; i<rk; ++i)
    coroot(i) = root(i);

  // complete with computation of non-simple positive coroots
  for (RootNbr alpha=rk; alpha<npos; ++alpha)
  {
    size_t i = ri[alpha].descents.firstBit();
    RootNbr beta = link[alpha][i];
    assert(beta<alpha); // so |coroot(beta)| is already defined
    coroot(alpha)=coroot(beta); // take a copy
    coroot(alpha)[i]-=coroot(beta).dot(simple_root[i]); // and modify
  }

  root_perm.resize(npos,Permutation(2*npos));
  // first fill in the simple root permutations
  for (size_t i=0; i<rk; ++i)
  {
    Permutation& perm=root_perm[i];
    for (RootNbr alpha=0; alpha<npos; ++alpha)
      if (alpha==i) // simple root reflecting itself makes it negative
      {
	perm[npos+alpha]=npos-1-alpha;
	perm[npos-1-alpha]=npos+alpha;
      }
      else // don't change positivity status
      {
	RootNbr beta = link[alpha][i];
	perm[npos+alpha] = npos+beta;
	perm[npos-1-alpha]=npos-1-beta;
      }
  }

  // extend permutations to all positive roots by conjugation from lower root
  for (size_t alpha=rk; alpha<npos; ++alpha)
  {
    size_t i=ri[alpha].descents.firstBit();
    assert(i<rk);
    Permutation& alpha_perm=root_perm[alpha];
    alpha_perm=root_perm[i]; // copy; this and next two statements alias-free
    root_perm[link[alpha][i]].renumber(alpha_perm);
    root_perm[i].renumber(alpha_perm);
  }

} // end of basic constructor

RootSystem::RootSystem(const RootSystem& rs, tags::DualTag)
  : rk(rs.rk)
  , Cmat(rs.Cmat) // transposed below
  , ri(rs.ri)     // entries modified internally in non simply laced case
  , two_rho_in_simple_roots(rs.two_rho_in_simple_roots) // similar
  , root_perm(rs.root_perm) // unchanged
{
  bool simply_laced = true;
  for (size_t i=0; i<rk; ++i)
    for (size_t j=i+1; j<rk; ++j) // do only case $i<j$, upper triangle
      if (Cartan_entry(i,j)!=Cartan_entry(j,i))
	simply_laced = false , std::swap(Cartan_entry(i,j),Cartan_entry(j,i));

  if (not simply_laced)
  {
    two_rho_in_simple_roots.assign(rk,0); // clear before recomputation
    for (RootNbr alpha=0; alpha<numPosRoots(); ++alpha)
    {
      root(alpha).swap(coroot(alpha)); // |descent|, |ascent| are OK
      const Byte_vector& a=root(alpha);
      for (size_t i=0; i<rk; ++i)
	two_rho_in_simple_roots[i]+=a[i]; // sum positive root expressions
    }
  }
} // end of dual constructor


int_Vector RootSystem::root_expr(RootNbr alpha) const
{
  RootNbr a=rt_abs(alpha);
  int_Vector expr(root(a).begin(),root(a).end());
  if (is_negroot(alpha))
    expr *= -1;
  return expr;
}

int_Vector RootSystem::coroot_expr(RootNbr alpha) const
{
  RootNbr a=rt_abs(alpha);
  int_Vector expr(coroot(a).begin(),coroot(a).end());
  if (is_negroot(alpha))
    expr *= -1;
  return expr;
}

int RootSystem::level(RootNbr alpha) const
{
  RootNbr a=rt_abs(alpha);
  int result=0;
  for (Byte_vector::const_iterator it=root(a).begin(); it!=root(a).end(); ++it)
    result += *it;
  if (is_negroot(alpha))
    result *= -1;
  return result;
}

int RootSystem::colevel(RootNbr alpha) const
{
  RootNbr a=rt_abs(alpha);
  int result=0;
  for (Byte_vector::const_iterator
	 it=coroot(a).begin(); it!=coroot(a).end(); ++it)
    result += *it;
  if (is_negroot(alpha))
    result *= -1;
  return result;
}

/*!
  In this template we assume that |I| is an |InputIterator| with |value_type|
  |RootNbr|, and that |O| is an |OutputIterator| with |value_type| |Weight|.
  We output via |out| the expressions of the roots numbered in |[first,last[|
  in the simple root basis.
*/
template <typename I, typename O>
  void RootSystem::toRootBasis(I first, I last, O out) const
{
  while (first!=last)
  {
    *out = root_expr(*first);
    ++out, ++first; // might be cheaper that post-increment for some iterators
  }
}

/*!
  In this template, |I| is an Input_iterator with |value_type| |RootNbr|, and
  |O| is an OutputIterator with |value_type| |Weight|. We assume that |rb|
  contains a basis of some _sub_ rootsystem of |rd|, and that |I| inputs
  |RootNbr|s corresponding to roots in that subsystem. Then we write to |out|
  the expression of the root in the basis |rb|.

  The idea is to use the coroots of the vectors in |rb| to get the expression
  of both the input roots and those from |rb| itself in the simple weight
  basis for |rb| (this is done by |toSimpleWeights| below); the latter matrix
  (the Cartan matrix of |rb|) is square, so we can then apply |baseChange|,
  which amounts to left-multiplication by the inverse of that Cartan matrix.
*/
template <typename I, typename O>
  void RootSystem::toRootBasis(I first, I last, O out, const RootNbrList& rb) const
{
  int_VectorList wl; // roots |[first,last[| in weight basis of |rb|
  int_VectorList wb; // (square) Cartan matrix of |rb|

  toSimpleWeights(first,last,back_inserter(wl),rb);
  toSimpleWeights(rb.begin(),rb.end(),back_inserter(wb),rb);

  lattice::baseChange(wl.begin(),wl.end(),out,wb.begin(),wb.end());
}

/*!
  In this template we assume that |I| is an InputIterator with value type
  |RootNbr|, that |O| is an OutputIterator with |value_type| |Weight|, and
  that |rb| flags a basis for some _sub_ rootsystem of our |RootSystem|. The
  range $[first,last)$ should access root numbers of roots in the subsystem.
  Then for each |v| in this range we output to |out| the expression of |v| in
  the simple weight basis of the root subsystem |rb|; this is obtained simply
  by taking scalar products with the coroots of the roots in |rb|.
*/
template <typename I, typename O>
  void RootSystem::toSimpleWeights
    (I first, I last, O out, const RootNbrList& rb) const
{
  int_Vector v(rb.size()); // temporary storage for vector

  while (first!=last)
  {
    for (unsigned long i=0; i<rb.size(); ++i)
      v[i] = bracket(*first,rb[i]); // $\<\alpha_{*first},\alpha_{rb[i]}^\vee>$
    *out = v;
    ++out, ++first;
  }
}

RootNbrSet RootSystem::simpleRootSet() const
{
  RootNbrSet simple_roots(numRoots());
  simple_roots.fill(numPosRoots(),numPosRoots()+rk);
  return simple_roots;
}

RootNbrList RootSystem::simpleRootList() const
{
  RootNbrList simple_roots(rk);
  for (weyl::Generator i=0; i<rk; ++i)
    simple_roots[i]=simpleRootNbr(i);
  return simple_roots;
}

RootNbrSet RootSystem::posRootSet() const
{
  RootNbrSet pos_roots(numRoots());
  pos_roots.fill(numPosRoots(),numRoots());
  return pos_roots;
}


int_Matrix RootSystem::cartanMatrix() const
{
  int_Matrix result(rk,rk);

  for (size_t i=0; i<rk; ++i)
    for (size_t j=0; j<rk; ++j)
      result(i,j) = Cartan_entry(i,j);

  return result;
}

// The Cartan matrix of the root subsystem with basis |rb|.
int_Matrix RootSystem::cartanMatrix(const RootNbrList& rb) const
{
  size_t r = rb.size();

  int_Matrix result(r,r);

  for (size_t i=0; i<r; ++i)
    for (size_t j=0; j<r; ++j)
      result(i,j) = bracket(rb[i],rb[j]);

  return result;
}

LieType RootSystem::Lie_type() const
{ return dynkin::Lie_type(cartanMatrix()); }

LieType RootSystem::Lie_type(RootNbrList sub) const
{ return dynkin::Lie_type(cartanMatrix(sub)); }

// pairing $\<\alpha,\beta^\vee>$; note that coroot is on the right
int RootSystem::bracket(RootNbr alpha, RootNbr beta) const
{
  if (isOrthogonal(alpha,beta)) // this is quick
    return 0;

/*
  Since |cartan_entry(i,j)| pairs simple root |i| and simple coroot |j| (our
  conventions would prefer the tranpose) we compute root(i)*Cartan*coroot(j)
 */
  const Byte_vector& row = root(rt_abs(alpha));
  const Byte_vector& col = coroot(rt_abs(beta));

  int c=0;
  for (size_t i=0; i<rk; ++i)
    for (size_t j=0; j<rk; ++j)
      c += row[i]*Cartan_entry(i,j)*col[j];

  return is_posroot(alpha)!=is_posroot(beta) ? -c : c;
}

Permutation
RootSystem::extend_to_roots(const RootNbrList& simple_image) const
{
  assert(simple_image.size()==rk);
  Permutation result(numRoots());

  RootNbrList image_reflection(rk);

  // prepare indexes of reflections for roots in |simple_image|
  for (size_t i=0; i<rk; ++i)
    image_reflection[i] = rt_abs(simple_image[i]);

  // copy images of simple roots
  for (size_t i=0; i<rk; ++i)
    result[simpleRootNbr(i)]=simple_image[i];

  // extend to positive roots
  for (size_t alpha=numPosRoots()+rk; alpha<numRoots(); ++alpha)
  {
    unsigned int i = ri[alpha-numPosRoots()].descents.firstBit();
    assert(i<rk);
    RootNbr beta = simple_reflected_root(i,alpha);
    assert(is_posroot(beta) and beta<alpha);
    result[alpha] = root_perm[image_reflection[i]][result[beta]];
  }

  // finally extend to negative roots, using symmetry of root permutation
  for (size_t alpha=0; alpha<numPosRoots(); ++alpha) // |alpha| negative root
    result[alpha]=rootMinus(result[rootMinus(alpha)]);

  return result;
}

Permutation
RootSystem::root_permutation(const Permutation& twist) const
{
  assert(twist.size()==rk);
  RootNbrList simple_image(rk);
  for (size_t i=0; i<twist.size(); ++i)
    simple_image[i] = simpleRootNbr(twist[i]);

  return extend_to_roots(simple_image);
}

WeylWord RootSystem::reflectionWord(RootNbr alpha) const
{
  make_positive(*this,alpha);

  WeylWord result; result.reserve(numPosRoots());

  while (alpha>=rk+numPosRoots()) // alpha positive but not simple
  {
    size_t i = ri[alpha-numPosRoots()].descents.firstBit();
    result.push_back(i);
    alpha = root_perm[i][alpha];
  }
  result.push_back(alpha-numPosRoots()); // central reflection
  for (size_t i=result.size()-1; i-->0;) // trace back to do conjugation
    result.push_back(result[i]);

  return result;
}


// |Delta| gives images of simple roots by an automorphism; find image of 2rho
matrix::Vector<int> RootSystem::pos_system_vec(const RootNbrList& Delta) const
{
  assert(Delta.size()==rk); // as many images as simple roots
  matrix::Vector<int> coef(rk,0); // coefficients of simple roots in result
  for (weyl::Generator s=0; s<rk; ++s) // compute matrix*vector for image
    coef += root_expr(Delta[s])*=two_rho_in_simple_roots[s]; // Delta*2rho
  // now |coef| holds image of $2\rho$ expressed as sum of simple roots

  // multiplication is from the right here because Cartan matrix is unnatural
  return cartanMatrix().right_prod(coef); // transform to fundamental weights
}

RootNbrList RootSystem::simpleBasis(RootNbrSet rs) const
{
  rs.clear(0,numPosRoots()); // clear any negative roots, not considered here
  RootNbrSet candidates = rs; // make a copy that will be pruned

  for (RootNbrSet::iterator it=candidates.begin(); it(); ++it)
  {
    RootNbr alpha=*it;
    for (RootNbrSet::iterator
	   jt=rs.begin(); jt(); ++jt) // traverse unpruned positive subsystem
    {
      RootNbr beta=*jt;
      if (alpha==beta) continue; // avoid reflecting root itself
      RootNbr gamma = root_perm[alpha-numPosRoots()][beta];
      if (gamma<beta) // positive dot product
      {
	if (is_posroot(gamma)) // beta can be made less positive, so it cannot
	  candidates.remove(beta); // be simple; remove it if it was candidate
	else
	{ // reflection in alpha makes some other root (beta) negative, so
	  candidates.remove(alpha); // alpha is not simple; remove it
	  break; // and move on to the next candadate
	}
      }
    } // |for (beta)|
    if (not candidates.isMember(alpha)) // so we just removed it, |break| again
      break;
  } // |for (alpha)|
  // now every reflection among |candidates| permutes the other members of |rs|

  return RootNbrList(candidates.begin(), candidates.end()); // convert to vector
}

bool RootSystem::sumIsRoot(RootNbr alpha, RootNbr beta, RootNbr& gamma) const
{
  RootNbr a = rt_abs(alpha);
  RootNbr b = rt_abs(beta);

  bool alpha_less = root_compare()(root(a),root(b)); // ordering among posroots
  if (alpha_less) // compare actual levels
    std::swap(a,b); // ensure |a| is higher root

  Byte_vector v =
    is_posroot(alpha)==is_posroot(beta)
    ? root(a) + root(b)
    : root(a) - root(b);

  for (RootNbr i=0; i<numPosRoots(); ++i) // search positive roots for |v|
    if (v==root(i))
    {
      gamma = posRootNbr(i);
      if (alpha_less ? is_negroot(beta) : is_negroot(alpha))
	gamma = rootMinus(gamma); // take sign from that root that gave |a|
      return true;
    }

  return false; // not found
}

/*
  Make the orthogonal system |rset| into an equaivalent (for |refl_prod|) one
  that is additively closed inside the full root system.

  This can be achieved by repeatedly replacing a pair of short roots spanning
  a B2 subsystem by a pair of long roots of that subsystem. Although we record
  no information about relative root lengths it is easily seen that the long
  roots in some B2 subsystem can never be short roots in another subsystem, so
  there is no need to assure newly created roots are inspected subsequently.
*/
RootNbrSet RootSystem::long_orthogonalize(const RootNbrSet& rset) const
{
  RootNbrSet result = rset;
  RootNbr gamma;
  for (RootNbrSet::iterator it=result.begin(); it(); ++it)
    for (RootNbrSet::iterator jt=result.begin(); jt!=it; ++jt)
      if (sumIsRoot(*it,*jt,gamma))
      { // replace *it and *jt by sum and difference (short->long in B2 system)
	result.insert(gamma);
	result.insert(root_permutation(*jt)[gamma]);
	result.remove(*it); result.remove(*jt);
	break; // move to next |*it|; without this inner loop won't terminate!
      }
  return result;
}


RootNbrList RootSystem::high_roots() const
{
  RootNbrList h;
  for (RootNbr alpha=0; alpha<numPosRoots(); ++alpha) // traverse negative roots
    if (descent_set(alpha).none())
      h.push_back(rootMinus(alpha));

  return h;
}



/*     The |RootDatum| class implementation     */



RootDatum::RootDatum(const PreRootDatum& prd)
  : RootSystem(prd.Cartan_matrix())
  , d_rank(prd.rank())
  , d_roots(numRoots())   // dimension only
  , d_coroots(numRoots()) // same for coroots
  , weight_numer(), coweight_numer()
  , d_radicalBasis(), d_coradicalBasis()
  , d_2rho(d_rank,0)
  , d_dual_2rho(d_rank,0)
  , Cartan_denom()
  , d_status()
{
  int_Matrix root_mat(prd.simple_roots(),d_rank);
  int_Matrix coroot_mat(prd.simple_coroots(),d_rank);
  for (RootNbr alpha=numPosRoots(); alpha<numRoots(); ++alpha)
  {
    d_roots[alpha]= root_mat*root_expr(alpha);
    d_coroots[alpha] = coroot_mat*coroot_expr(alpha);
    d_2rho += d_roots[alpha];
    d_dual_2rho += d_coroots[alpha];
    d_roots[rootMinus(alpha)] = -d_roots[alpha];
    d_coroots[rootMinus(alpha)] = -d_coroots[alpha];
  }


  // the fundamental weights are given by the matrix Q.tC^{-1}, where Q is
  // the matrix of the simple roots, tC the transpose Cartan matrix
  int_Matrix iC = cartanMatrix().inverse(Cartan_denom);
  weight_numer = (root_mat*iC.transposed()).columns();

  // for the fundamental coweights, use coroots and (untransposed) Cartan matrix
  coweight_numer = (coroot_mat*iC).columns();

  // get basis of co-radical character lattice, if any (or leave empty list)
  if (semisimpleRank()<d_rank)
  {
    d_coradicalBasis = lattice::perp(prd.simple_coroots(),d_rank);
    d_radicalBasis   = lattice::perp(prd.simple_roots(),d_rank);
  }

  // fill in the status
  fillStatus();
}



/*
  Construct the root system dual to the given one.

  Since we do not use distinct types for weights and coweights, we can proceed
  by interchanging roots and coroots. The ordering of the roots corresponds to
  that of the original root datum (root |i| of the dual datum is coroot |i| of
  the orginal datum; this is not the ordering that would have been used in a
  freshly constructed root datum), but users should \emph{not} depend on this.
*/
RootDatum::RootDatum(const RootDatum& rd, tags::DualTag)
  : RootSystem(rd,tags::DualTag())
  , d_rank(rd.d_rank)
  , d_roots(rd.d_coroots)
  , d_coroots(rd.d_roots)
  , weight_numer(rd.coweight_numer)
  , coweight_numer(rd.weight_numer)
  , d_radicalBasis(rd.d_coradicalBasis)
  , d_coradicalBasis(rd.d_radicalBasis)
  , d_2rho(rd.d_dual_2rho)
  , d_dual_2rho(rd.d_2rho)
  , Cartan_denom(rd.Cartan_denom)
  , d_status()
{
  // fill in the status

  fillStatus();

  assert(d_status[IsAdjoint] == rd.d_status[IsSimplyConnected]);
  assert(d_status[IsSimplyConnected] == rd.d_status[IsAdjoint]);
}

/* Construct the derived root datum, and put weight mapping into |projector| */

RootDatum::RootDatum(int_Matrix& projector, const RootDatum& rd,
		     tags::DerivedTag)
  : RootSystem(rd)
  , d_rank(rd.semisimpleRank())
  , d_roots(rd.numRoots())
  , d_coroots(rd.numRoots())
  , weight_numer(d_rank)
  , coweight_numer(d_rank)
  , d_radicalBasis(), d_coradicalBasis() // these remain empty
  , d_2rho()
  , d_dual_2rho()
  , Cartan_denom(rd.Cartan_denom)
  , d_status()
{
  size_t r=rd.rank(), d=r-d_rank;
  int_Matrix kernel // of restriction map from weights to derived weights
    (rd.beginCoradical(),rd.endCoradical(),r,tags::IteratorTag());

  assert(kernel.numColumns()==d);

  int_Matrix row,col;
  matreduc::diagonalise(kernel,row,col); // factors (|d| times 1) not needed

  // the restriction map is surjective, and therefore called |projector|
  projector = row.block(d,0,r,r); // cokernel of |kernel|, projects weights
  int_Matrix section // will satisfy $projector*section=Id_d$;
    = row.inverse().block(0,d,r,r); // transforms coweights by right-action

  for (RootNbr i=0; i<rd.numRoots(); ++i)
  {
    d_roots[i] = projector*rd.d_roots[i];
    d_coroots[i] = section.right_prod(rd.d_coroots[i]);
  }

  d_2rho = projector*rd.d_2rho;
  d_dual_2rho = section.right_prod(rd.d_dual_2rho);

  fillStatus();
} // |RootDatum::RootDatum(...,DerivedTag)|

/* Construct the adjoint root datum, and put weight mapping into |injector| */

RootDatum::RootDatum(int_Matrix& injector, const RootDatum& rd,
		     tags::AdjointTag)
  : RootSystem(rd)
  , d_rank(rd.semisimpleRank())
  , d_roots(rd.numRoots())
  , d_coroots(rd.numRoots())
  , weight_numer(d_rank)
  , coweight_numer(d_rank)
  , d_radicalBasis(), d_coradicalBasis() // these remain empty
  , d_2rho()
  , d_dual_2rho()
  , Cartan_denom(rd.Cartan_denom)
  , d_status()
{
  size_t r=rd.rank(), d=r-d_rank;
  int_Matrix kernel // of restriction map from coweights to adjoint coweights
    (rd.beginRadical(),rd.endRadical(),r,tags::IteratorTag());

  assert(kernel.numColumns()==d);

  int_Matrix row,col;
  matreduc::diagonalise(kernel,row,col); // factors (|d| times 1) not needed

  // the restriction map is surjective, its transpose is called |injector|
  injector = row.block(d,0,r,r).transposed(); // maps adjoint weights to wts
  int_Matrix section // will satisfy $section^t*injector=Id_d$;
    = row.inverse().block(0,d,r,r); // maps adjoint coweights to coweights

  for (RootNbr i=0; i<rd.numRoots(); ++i)
  {
    d_roots[i] = section.right_prod(rd.d_roots[i]);
    d_coroots[i] = injector.right_prod(rd.d_coroots[i]);
  }

  d_2rho = section.right_prod(rd.d_2rho);
  d_dual_2rho = injector.right_prod(rd.d_dual_2rho);

  fillStatus();
} // |RootDatum::RootDatum(...,AdjointTag)|



RootDatum RootDatum::sub_datum(const RootNbrList& generators) const
{
  WeightList roots;     roots.reserve(generators.size());
  CoweightList coroots; coroots.reserve(generators.size());
  for (RootNbrList::const_iterator it=
	 generators.begin(); it!=generators.end(); ++it)
  {
    roots.push_back(d_roots[*it]);
    coroots.push_back(d_coroots[*it]);
  }

  return RootDatum(PreRootDatum(roots,coroots,rank()));
}


RatWeight RootDatum::fundamental_weight(weyl::Generator i) const
{ return RatWeight(weight_numer[i],Cartan_denom); }

RatWeight RootDatum::fundamental_coweight(weyl::Generator i) const
{ return RatWeight(coweight_numer[i],Cartan_denom); }

/******** accessors **********************************************************/

void RootDatum::reflect(RootNbr alpha, LatticeMatrix& M) const
{
  assert(M.numRows()==rank());
  for (unsigned int j=0; j<M.numColumns(); ++j)
  {
    int s=0;
    for (unsigned int i=0; i<rank(); ++i)
      s+= coroot(alpha)[i]*M(i,j);
    for (unsigned int i=0; i<rank(); ++i)
      M(i,j) -= root(alpha)[i]*s;
  }
}

void RootDatum::reflect(LatticeMatrix& M,RootNbr alpha) const
{
  assert(M.numColumns()==rank());
  for (unsigned int i=0; i<M.numRows(); ++i)
  {
    int s=0;
    for (unsigned int j=0; j<rank(); ++j)
      s+= M(i,j)*root(alpha)[j];
    for (unsigned int j=0; j<rank(); ++j)
      M(i,j) -= s*coroot(alpha)[j];
  }
}


/*
  Return the permutation of the roots induced by |q|.

  Precondition: |q| permutes the roots;
*/
Permutation RootDatum::rootPermutation(const WeightInvolution& q) const
{
  RootNbrList simple_image(semisimpleRank());

  for (weyl::Generator s=0; s<semisimpleRank(); ++s)
  { auto image = root_index(q*simpleRoot(s));
    assert(image<numRoots());
    simple_image[s] = image;
  }

  return extend_to_roots(simple_image);
}



// Return the reflection for root number |alpha|.
WeightInvolution RootDatum::root_reflection(RootNbr alpha) const
{
  LatticeMatrix result(d_rank); // identity

  // do |result -= root(alpha).column_matrix() * coroot(alpha).row_matrix();|
  const Root& root = d_roots[alpha];
  const Root& coroot = d_coroots[alpha];

  for (size_t i=0; i<d_rank; ++i)
    for (size_t j=0; j<d_rank; ++j)
      result(i,j) -= root[i]*coroot[j];

  return result;
}

WeylWord RootDatum::reflectionWord(RootNbr alpha) const
{
  return to_dominant(reflection(alpha,twoRho()));
}



/*
  The expression of $q^{-1}$ as a product of simple reflections.

  Precondition: $q$ gives the action on the weight lattice of some Weyl group
  element

  Algorithm: apply $q$ to |twoRho|, then use |toPositive| to find a Weyl
  word making it dominant again, which by assumption expresses $q^{-1}$.
*/
WeylWord RootDatum::word_of_inverse_matrix
  (const WeightInvolution& q) const
{
  return to_dominant(q*twoRho());
}

/*
  The sum of the positive roots in rl.

  Precondition: rl holds the roots in a sub-rootsystem of the root system of
  rd;
*/
Weight RootDatum::twoRho(const RootNbrList& rl) const
{
  Weight result(rank(),0);

  for (size_t i = 0; i < rl.size(); ++i)
    if (is_posroot(rl[i]))
      result += root(rl[i]);

  return result;
}

/* Returns the sum of the positive roots in rs.

  Precondition: rs holds the roots in a sub-rootsystem of the root system of
  rd, or possibly only the positive roots in such a subsystem
*/
Weight RootDatum::twoRho(RootNbrSet rs) const
{
  Weight result(rank(),0);
  rs &= posRootSet(); // limit to positive roots in the subset

  for (RootNbrSet::iterator i = rs.begin(); i(); ++i)
    result += root(*i);

  return result;
}

// and their dual relatives
Coweight RootDatum::dual_twoRho(const RootNbrList& rl) const
{
  Coweight result(rank(),0);

  for (size_t i = 0; i < rl.size(); ++i)
    if (is_posroot(rl[i]))
      result += coroot(rl[i]);

  return result;
}

Coweight RootDatum::dual_twoRho(RootNbrSet rs) const
{
  Coweight result(rank(),0);
  rs &= posRootSet(); // limit to positive roots in the subset

  for (RootNbrSet::iterator i = rs.begin(); i(); ++i)
    result += coroot(*i);

  return result;
}


/*
  A reduced expression of the shortest |w| making |w.v| dominant

  Algorithm: the greedy algorithm -- if v is not positive, there is a
  simple coroot alpha^v such that <v,alpha^v> is < 0; then s_alpha.v takes
  v closer to the dominant chamber.
*/
WeylWord RootDatum::to_dominant(Weight v) const
{
  WeylWord result;

  weyl::Generator i;
  do
    for (i=0; i<semisimpleRank(); ++i)
      if (v.dot(simpleCoroot(i)) < 0)
      {
	result.push_back(i);
	simple_reflect(i,v);
	break;
      }
  while (i<semisimpleRank());

  // reverse result (action is from right to left)
  std::reverse(result.begin(),result.end());
  return result;
}

// the same algorithm, but now modifying |v| in place, forgetting |w|
Weight& RootDatum::make_dominant(Weight& v) const
{
  weyl::Generator i;
  do
    for (i=0; i<semisimpleRank(); ++i)
      if (v.dot(simpleCoroot(i)) < 0)
      {
	simple_reflect(i,v);
	break;
      }
  while (i<semisimpleRank());

  return v;
}

/*
  The matrix represented by ww.

  NOTE: not intended for heavy use. If that were the case, it would be better
  to use the decomposition of ww into pieces, and multiply those together.
  However, that would be for the InnerClass to do, as it is it that has
  access to both the Weyl group and the root datum.
*/
LatticeMatrix RootDatum::action_matrix(const WeylWord& ww) const
{
  LatticeMatrix result(rank());
  for (size_t i=0; i<ww.size(); ++i)
    result *= simple_reflection(ww[i]);
  return result;
}



/******** manipulators *******************************************************/

void RootDatum::swap(RootDatum& other)
{
  std::swap(d_rank,other.d_rank);
  d_roots.swap(other.d_roots);
  d_coroots.swap(other.d_coroots);
  weight_numer.swap(other.weight_numer);
  coweight_numer.swap(other.coweight_numer);
  d_radicalBasis.swap(other.d_radicalBasis);
  d_coradicalBasis.swap(other.d_coradicalBasis);
  d_2rho.swap(other.d_2rho);
  d_dual_2rho.swap(other.d_dual_2rho);
  std::swap(Cartan_denom,other.Cartan_denom);
  d_status.swap(other.d_status);
}

/******** private member functions ******************************************/


/*
  Fill in the status of the rootdatum.

  The status flag |IsAdjoint| indicates whether the center of the complex
  group determined by the root datum is connected. This means that the root
  lattice is a saturated sublattice of $X^*$. Dually |IsSimplyConnected|
  indicates whether the derived group is simply connected, which means that
  the coroots span a saturated sublattice of $X_*$.
*/
void RootDatum::fillStatus()
{
  int_Matrix
    q(beginSimpleRoot(),endSimpleRoot(),rank(),tags::IteratorTag());

  int_Matrix row,col; // their values remain unused
  CoeffList factor = matreduc::diagonalise(q,row,col);

  d_status.set(IsAdjoint); // remains set only of all |factor|s are 1

  for (size_t i=0; i<factor.size(); ++i)
    if (std::abs(factor[i]) != 1)
      d_status.reset(IsAdjoint);

  q = int_Matrix
     (beginSimpleCoroot(),endSimpleCoroot(),rank(),tags::IteratorTag());

  factor = matreduc::diagonalise(q,row,col); // redo computation on dual side

  d_status.set(IsSimplyConnected); // remains set only of all |factor|s are 1

  for (size_t i=0; i<factor.size(); ++i)
    if (std::abs(factor[i]) != 1)
      d_status.reset(IsSimplyConnected);
}



/*****************************************************************************

        Chapter II -- Functions declared in rootdata.h

******************************************************************************/

RatWeight rho (const RootDatum& rd) { return RatWeight(rd.twoRho(),2); }
RatWeight rho (const RootDatum& rd, const RootNbrSet& sub_posroots)
  { return RatWeight(rd.twoRho(sub_posroots),2); }
RatCoweight rho_check (const RootDatum& rd)
  { return RatCoweight(rd.dual_twoRho(),2); }
RatCoweight rho_check (const RootDatum& rd, const RootNbrSet& sub_posroots)
  { return RatCoweight(rd.dual_twoRho(sub_posroots),2); }

/*
  Return matrix of dual fundamental involution related to fundamental |q|

  Here |q| is an involution of |rd| as a _based_ root datum.
  Note that |rd| is not the dual root datum, although the result will be a
  based involution for that dual datum

  Returns an involution of the based root datum dual to |rd|

  Formula: it is given by $(w_0^t)(-q^t) = (-q.w_0)^t$

  In other words, to |-q| we apply the (longest) Weyl group element $w_0$
  (making the image of the dominant chamber dominant again) and transpose it

  Since $-w_0$ is central in the group of based root datum automorphisms, it
  doesn't matter whether one multiplies by $w_0$ on the left or on the right
*/
CoweightInvolution dualBasedInvolution
  (const WeightInvolution& q, const RootDatum& rd)
{
  WeylWord w0 = rd.to_dominant(-rd.twoRho());
  return (q*rd.action_matrix(w0)).negative_transposed();
}


// The elements of |subsys| which are orthogonal to all elements of |o|.
RootNbrSet makeOrthogonal(const RootNbrSet& o, const RootNbrSet& subsys,
			  const RootSystem& rs)
{
  RootNbrSet candidates = subsys;
  candidates.andnot(o); // roots of |o| itself need not be considered

  RootNbrSet result = candidates;

  for (RootNbrSet::iterator it = candidates.begin(); it(); ++it)
  {
    RootNbr alpha = *it;
    RootNbrSet::iterator jt;
    for (jt = o.begin(); jt(); ++jt)
      if (not rs.isOrthogonal(alpha,*jt))
      {
	result.remove(alpha);
	break;
      }
  }

  return result;
}


/*
  Transform $q$, assumed a root datum involution, into a based root
  datum involution $w.q,$ which fixes the positive Weyl chamber.

  Note that wq is then automatically an involution permuting the simple roots
*/

void toDistinguished(WeightInvolution& q, const RootDatum& rd)
{ // easy implementation; using |simple_root_permutation| is more efficient
  Weight v = q*rd.twoRho();
  q.leftMult(rd.action_matrix(rd.to_dominant(v)));
}

/*
   Transform, using the Weyl group, the image |Delta| of simple system back to
   that system as a set, possibly permuted by a twist. At return |Delta|
   represents that twist, and return value transforms it to original |Delta|
 */
WeylWord wrt_distinguished(const RootSystem& rs, RootNbrList& Delta)
{
  WeylWord result;
  const size_t rank=rs.rank();
  matrix::Vector<int> v = rs.pos_system_vec(Delta); // image of $2\rho$
  weyl::Generator s;
  do
    for (s=0; s<rank; ++s)
      if (v[s]<0)
      {
	result.push_back(s);
	int c=-v[s]; // save scalar factor which is modified halfway in loop
	for (size_t j=0; j<rank; ++j) // apply reflection |s| to |v|
	  v[j]+=c*rs.cartan(s,j);
	break;
      }
  while (s<rank);

  for (size_t i=0; i<rank; ++i)
    Delta[i]=rs.permuted_root(Delta[i],result);
    // should be simple, but don't assert that here, to allow a test for it

  return result;
}

void make_positive(const RootSystem& rs,RootNbr& alpha)
{
  if (rs.is_negroot(alpha))
    alpha = rs.rootMinus(alpha);
}

// conjugate |alpha| to a simple root, returning right-conjugating word applied
// afterwards |alpha| is shifted to become a \emph{simple} root index
WeylWord conjugate_to_simple(const RootSystem& rs,RootNbr& alpha)
{
  make_positive(rs,alpha);
  WeylWord result;
  result.reserve(4*rs.rank()); // generous size that avoids reallocations
  weyl::Generator s;
  while (alpha!=rs.simpleRootNbr(s=rs.find_descent(alpha)))
  {
    result.push_back(s);
    rs.simple_reflect_root(s,alpha);
  }
  alpha=s; // alpha takes on simple root numbering from now on!
  return result;
}

// set of positive roots made negative by left multiplication by |w|
// for letter s in w (L to R): reflect by by $\alpha_s$, which posroot toggle
RootNbrSet pos_to_neg (const RootSystem& rs, const WeylWord& w)
{ RootNbr npos = rs.numPosRoots();
  std::vector<Permutation> pos_perm_simple;
  pos_perm_simple.reserve(rs.rank());
  for (unsigned i=0; i<rs.rank(); ++i)
  { const Permutation& p = rs.simple_root_permutation(i);
    pos_perm_simple.push_back(Permutation(npos));
    for (RootNbr j=0; j<npos; ++j)
      if (j==i) pos_perm_simple.back()[j]=i; // make |j| itself a fixed point
      else pos_perm_simple.back()[j]=rs.posRootIndex(p[rs.posRootNbr(j)]);
  }

  RootNbrSet current(npos), tmp(npos);
  for (auto it=w.begin(); it!=w.end(); ++it)
  { tmp.reset(); // clear out all old bits
    Permutation& p=pos_perm_simple[*it]; // a positive root permutation
    for (auto root_it=current.begin(); root_it(); ++root_it)
      tmp.insert(p[*root_it]); // apply permutation to the subset of roots
    tmp.flip(*it); // flip status of simple root for current letter of |w|
    current=tmp;
  }

  current.set_capacity(rs.numRoots()); // double the size
  return current <<= npos; // shift to root (rather than posroot) numbers
}

/*
  Return the matrix represented by the product of the
  reflections for the set of roots |rset|.

  The roots must be mutually orthogonal so that the order doesn't matter.
*/
WeightInvolution refl_prod(const RootNbrSet& rset, const RootDatum& rd)
{
  WeightInvolution q(rd.rank()); // identity
  for (RootNbrSet::iterator it=rset.begin(); it(); ++it)
    q *= rd.root_reflection(*it);
  return q;
}


RootDatum integrality_datum(const RootDatum& rd, const RatWeight& gamma)
{
  arithmetic::Numer_t n=gamma.denominator(); // signed type!
  const Ratvec_Numer_t& v=gamma.numerator();
  RootNbrSet int_roots(rd.numRoots());
  for (size_t i=0; i<rd.numPosRoots(); ++i)
    if (rd.posCoroot(i).dot(v)%n == 0)
      int_roots.insert(rd.posRootNbr(i));

  return rd.sub_datum(rd.simpleBasis(int_roots));
}

unsigned int integrality_rank(const RootDatum& rd, const RatWeight& gamma)
{
  arithmetic::Numer_t n=gamma.denominator(); // signed type!
  const Ratvec_Numer_t& v=gamma.numerator();
  RootNbrSet int_roots(rd.numRoots());
  for (size_t i=0; i<rd.numPosRoots(); ++i)
    if (rd.posCoroot(i).dot(v)%n == 0)
      int_roots.insert(rd.posRootNbr(i));

  return rd.simpleBasis(int_roots).size();
}

RationalList integrality_points(const RootDatum& rd, const RatWeight& gamma)
{
  arithmetic::Denom_t d = gamma.denominator(); // unsigned type is safe here

  std::set<arithmetic::Denom_t> products;
  for (size_t i=0; i<rd.numPosRoots(); ++i)
  {
    arithmetic::Denom_t p = std::abs(rd.posCoroot(i).dot(gamma.numerator()));
    if (p!=0)
      products.insert(p);
  }

  std::set<Rational> fracs;
  for (std::set<arithmetic::Denom_t>::iterator
	 it= products.begin(); it!=products.end(); ++it)
    for (arithmetic::Denom_t s=d; s<=*it; s+=d)
      fracs.insert(Rational(s,*it));

  return RationalList(fracs.begin(),fracs.end());
}

weyl::Twist twist (const RootDatum& rd, const WeightInvolution& delta)
{ return weyl::Twist(fold_orbits(rd,delta)); }

ext_gens fold_orbits (const RootDatum& rd, const WeightInvolution& delta)
{
  ext_gens result;
  const Permutation pi = rd.rootPermutation(delta);
  for (weyl::Generator s=0; s<rd.semisimpleRank(); ++s)
  {
    RootNbr alpha=rd.simpleRootNbr(s);
    if (pi[alpha]==alpha)
      result.push_back(ext_gen(s));
    else if (pi[alpha]>alpha)
    {
      if (rd.is_simple_root(pi[alpha]))
	result.push_back(ext_gen(rd.isOrthogonal(alpha,pi[alpha]),
				 s,rd.simpleRootIndex(pi[alpha])));
      else
	throw std::runtime_error("Not a distinguished involution");
    }
  }
  return result;
}

RankFlags singular_generators(const RootDatum& rd, const RatWeight& gamma)
{
  const Ratvec_Numer_t& v=gamma.numerator();
  RankFlags result;
  for (weyl::Generator s=0; s<rd.semisimpleRank(); ++s)
    result.set(s,rd.simpleCoroot(s).dot(v) == 0);

  return result;
}

bool is_dominant_ratweight(const RootDatum& rd, const RatWeight& gamma)
{
  auto& numer = gamma.numerator();
  for (weyl::Generator s=0; s<rd.semisimpleRank(); ++s)
    if (rd.simpleCoroot(s).dot(numer)<0)
      return false;
  return true;
}

Weight rho_minus_w_rho(const RootDatum& rd, const WeylWord& ww)
{ Weight sum(rd.rank(),0);
  for (size_t i=0; i<ww.size(); ++i)
  { RootNbr alpha=rd.simpleRootNbr(ww[i]);
    for (size_t j=i; j-->0; )
      rd.simple_reflect_root(ww[j],alpha);
    sum += rd.root(alpha);
  }
  return sum;
}

Coweight rho_check_minus_rho_check_w(const RootDatum& rd, const WeylWord& ww)
{ Coweight sum(rd.rank(),0);
  for (size_t i=ww.size(); i-->0; )
  { RootNbr alpha=rd.simpleRootNbr(ww[i]);
    for (size_t j=i+1; j<ww.size(); ++j)
      rd.simple_reflect_root(ww[j],alpha);
    sum += rd.coroot(alpha);
  }
  return sum;
}

Weight root_sum(const RootDatum& rd, const RootNbrSet& S)
{ Weight sum(rd.rank(),0);
  for (auto it=S.begin(); it(); ++it)
    sum += rd.root(*it);
  return sum;
}

Coweight coroot_sum(const RootDatum& rd, const RootNbrSet& S)
{ Coweight sum(rd.rank(),0);
  for (auto it=S.begin(); it(); ++it)
    sum += rd.coroot(*it);
  return sum;
}

/*****************************************************************************

                Chapter III -- Auxiliary methods.

******************************************************************************/


// a class for making a compare object for indices, backwards lexicographic
class weight_compare
{
  const WeightList& alpha; // weights being compared
  CoweightList phi; // coweights by increasing priority

public:
  weight_compare(const WeightList& roots, const CoweightList& f)
    : alpha(roots), phi(f) {}

  void add_coweight(const Coweight& f) { phi.push_back(f); }

  bool operator() (size_t i, size_t j)
  {
    int x,y;
    for (size_t k=phi.size(); k-->0; )
      if ((x=phi[k].dot(alpha[i])) != (y=phi[k].dot(alpha[j])))
	return x<y;

    return false; // weights compare equal under all coweights
  }
}; // |class weight_compare|

template
void RootSystem::toRootBasis
  (RootNbrList::const_iterator,
   RootNbrList::const_iterator,
   std::back_insert_iterator<int_VectorList>) const;

template
void RootSystem::toRootBasis
  (RootNbrSet::const_iterator,
   RootNbrSet::const_iterator,
   std::back_insert_iterator<int_VectorList>,
   const RootNbrList&) const;
// this also  implicitly instantiates |RootSystem::toWeightBasis| twice

} // |namespace rootdata|

} // |namespace atlas|
