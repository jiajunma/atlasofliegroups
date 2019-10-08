/*
  Template definitions for the classes |Subspace| and |Subquotient|.

  These class templates deal with subspaces and subquotients of vector spaces
  over $Z/2Z$, elements of which are of type |BitVector|.
*/
/*
  This is subquotient.cpp

  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Lie Groups and Representations

  For license information see the LICENSE file
*/

#include "subquotient.h"

#include <cassert>
#include "bitset.h"

#include "../Atlas.h"

namespace atlas {

/*****************************************************************************

        Chapter I -- The Subspace class

*****************************************************************************/

namespace subquotient {


/*
  Construct the normalised subspace generated by the collection
  of |BitVectors| |b|, each of which has rank |n|.
*/
template<unsigned int dim> Subspace<dim>::
Subspace(const BitVectorList<dim>& b, size_t n)
  : d_basis(b)
  , d_support()
  , d_rank(n)
{
  assert(b.size()==0 or b[0].size()==n);

  // change |d_basis| and set |d_support|:
  bitvector::Gauss_Jordan(d_support,d_basis);
}

/*
  Construct the normalised subspace generated by the columns of
  a |BitMatrix|, i.e., the image of that matrix
*/
template<unsigned int dim> Subspace<dim>::
Subspace(const BitMatrix<dim>& M)
  : d_basis(M.image())
  , d_support()
  , d_rank(M.numRows())
{
 // change |d_basis| to a normalised form, and set |d_support|:
  bitvector::Gauss_Jordan(d_support,d_basis);
}

/******** accessors **********************************************************/

/*
  For the perp subspace, each element of |d_basis| serves as a homogeneous
  equation. For each position $j$ not in |support()| there is a solution
  generator, which has 1 in in position $j$, 0 in the other non-support
  positions, and all positions in |support()| solved by using the
  corresponding equation from |d_basis|. Since in characteristic 2 signs are
  ignored, this amounts to simply copying the respective bits at position $j$
  from |d_basis| to the positions in |support()|.
 */
template<unsigned int dim>
  BitVectorList<dim> Subspace<dim>::basis_perp () const
{
  BitSet<dim> new_supp = support_perp();
  BitVectorList<dim> result;
  result.reserve(new_supp.count());
  for (typename BitSet<dim>::iterator jt=new_supp.begin(); jt(); ++jt)
  {
    const unsigned j=*jt;
    BitVector<dim> gen(rank(),j);
    unsigned i=0; // index into |d_basis|
    for (typename BitSet<dim>::iterator it=d_support.begin(); it(); ++it,++i)
      gen.set(*it,d_basis[i][j]); // copy one bit
    result.push_back(gen);
  }
  return result;
}

template<unsigned int dim>
  bool Subspace<dim>::contains(const BitVector<dim>& v) const
    { return mod_image(v).isZero(); }

template<unsigned int dim>
  bool Subspace<dim>::contains(const BitVectorList<dim>& m) const
    {
      for (size_t i=0; i<m.size(); ++i)
	if (not contains(m[i])) return false;
      return true;
    }

/*
  Return canonical representative of |v| modulo our subspace.

  This representative is characterized by the vanishing of its bits in
  positions |d_support| (the leading bits of the canonical basis of our
  subspace). Note that we are making no reference to the space from which our
  subspace is divided out; this could be the whole space $(Z/2Z)^n$ (in any
  case |r| is expressed in its canonical basis) or some subspace containing
  our subspace.

  The algorithm projects v onto the subspace, the result |pv| being expressed
  in the canonical basis of the subspace: the coordinates of pv are just the
  entries of |v| in the coordinates flagged by |d_support|. Then |w| is
  computed as the corresponding |BitVector| in $(Z/2Z)^n$, given by combining
  the canonical basis with coefficients from |pv|.  Finally we set |r=v-w|.
*/
template<unsigned int dim>
BitVector<dim> Subspace<dim>::representative
  (const BitVector<dim>& v) const
{
  assert(v.size()==rank());

  // get projection to subspace expressed in d_basis
  BitVector<dim> pv = v;
  pv.slice(d_support);

  assert(pv.size()==dimension()); // |slice| set it to |d_support.count()|

  // expand that linear combination to an element of $(Z/2Z)^n$
  BitVector<dim> w = combination(d_basis,rank(),pv.data());

  assert(w.size()==rank());

  return v+w; // set |r| to original vector minus (or plus) correction |w|
}

/******** manipulators *******************************************************/


/*!
  Synopsis: applies |r| to our subspace, normalizing the resulting subspace.

  Precondition: r is a matrix of the appropriate column size (it could go to a
  space of another dimension, even.)
*/
template<unsigned int dim>
void Subspace<dim>::apply(const BitMatrix<dim>& r)
{
  assert(r.numColumns()==d_rank);

  BitVectorList<dim> b; b.reserve(dimension());

  for (size_t j = 0; j<dimension(); ++j)
  {
    b.push_back(r*d_basis[j]);
    assert(b.back().size()==r.numRows());
  }

  Subspace<dim> ns(b,r.numRows()); // construct (and normalize) new subspace
  swap(ns); // and install it in our place
}

template<unsigned int dim>
void Subspace<dim>::swap(Subspace& source)

{
  std::swap(d_rank,source.d_rank);
  d_basis.swap(source.d_basis);
  d_support.swap(source.d_support);
}

}

/*****************************************************************************

        Chapter II -- The Subquotient class

*****************************************************************************/

namespace subquotient {

// construct the subquotient of span of |bsp| by span of |bsub|
template<unsigned int dim> Subquotient<dim>::
Subquotient(const BitVectorList<dim>& bsp,
	    const BitVectorList<dim>& bsub, size_t n)
  : d_space(bsp,n)
  , d_subspace(bsub,n)
  , d_rel_support()
{
  assert(d_space.contains(bsub)); // check containment of subspaces
  assert(d_space.support().contains(d_subspace.support()));

  d_rel_support = d_space.support() - d_subspace.support(); // difference set
  d_rel_support.slice(d_space.support()); // only keep bits originally set

}

/******** manipulators *******************************************************/


// Apply matrix |r| to both spaces in the subquotient
template<unsigned int dim>
void Subquotient<dim>::apply(const BitMatrix<dim>& r)
{
  d_space.apply(r);
  d_subspace.apply(r);

  // recompute the relative support
  d_rel_support = d_space.support() - d_subspace.support();
  d_rel_support.slice(d_space.support());
}

template<unsigned int dim>
void Subquotient<dim>::swap(Subquotient& source)

{
  d_space.swap(source.d_space);
  d_subspace.swap(source.d_subspace);
  d_rel_support.swap(source.d_rel_support);
}

}

/*****************************************************************************

        Chapter III -- Functions declared in subquotient.h

*****************************************************************************/

namespace subquotient {

/*!
  Synopsis: puts in |msq| the matrix of the map induced by |m| at the
  subquotient level.

  Precondition: |m| has |source.rank()| columns and |dest.rank()| rows, in
  other words it defines a map at the level of the ambient $Z/2Z$-vector
  spaces. Moreover it maps |source.space()| to |dest.space()| and
  |source.denominator()| to |dest.denominator()|. [This is a strong condition, which
  it is the caller's responsibility to ensure. It is _not_ sufficient that |m|
  gives rise to a mathematically well-defined map between the subquotients;
  for instance while for any subspaces $A,B$ there is a canonical isomorphism
  between the subquotients $(A+B)/A$ and $B/(A\cap B)$, the identity matrix
  might not produce such an isomorphism when applied from the former to the
  latter subquotient (although it would in the opposite direction). MvL]

  The subquotient matrix will be expressed in terms of the canonical bases for
  the subquotients (which are made up of those elements of the basis of the
  larger subspace |space()| that are zero on the bits supporting the smaller
  subspace |denominator()|.) Therefore its size will be |source.dimension()|
  columns and |dest.dimension()| rows.

  Algorithm: take the image through |m| of the subquotient basis in |source|;
  then project onto the subquotient basis in |dest|. Note that this procedure
  inspects neither |source.denominator()| nor |dest.space()|, but it does select
  using |source.support()| those basis vectors from |source.space()| that
  cannot be reduced modulo |source.denominator()|, forming a complementary space.
*/
template<unsigned int dim>
  BitMatrix<dim> subquotientMap
    (const Subquotient<dim>& source,
     const Subquotient<dim>& dest,
     const BitMatrix<dim>& m)
{
  assert(m.numColumns()==source.rank());
  assert(m.numRows()==dest.rank());

  BitMatrix<dim> result(dest.dimension(),0);

  // restrict m to source.space()
  for (RankFlags::iterator it=source.support().begin(); it(); ++it)
  {
    SmallBitVector v = m*source.space().basis(*it);
    assert(v.size()==dest.rank());

    /*
    // go to canonical representative modulo destination subspace
    dest.denominator().mod_reduce(v);
    assert(v.size()==dest.rank());

    // get coordinates in canonical basis
    v.slice(dest.space().support());  // express |v| in basis of |d_space|
    assert(v.size()==dest.space().dimension());

    v.slice(dest.support());
    */

    v=dest.toBasis(v);
    assert(v.size()==dest.dimension()); // dimension of the subquotient

    result.addColumn(v);
  }
  return result;
}

template
  BitMatrix<constants::RANK_MAX> subquotientMap
   (const Subquotient<constants::RANK_MAX>&,
    const Subquotient<constants::RANK_MAX>&,
    const BitMatrix<constants::RANK_MAX>&);

template class Subspace<constants::RANK_MAX>;
template class Subquotient<constants::RANK_MAX>;

} // |namespace subquotient|

} // |namespace atlas|
