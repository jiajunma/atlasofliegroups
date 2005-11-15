/*
  This is subquotient_def.h
  
  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups version 0.2.3 

  See file main.cpp for full copyright notice
*/

#include <cassert>

namespace atlas {

/*****************************************************************************

        Chapter I -- The NormalSubspace class

  ... explain here when it is stable ...

*****************************************************************************/

namespace subquotient {

template<size_t dim> NormalSubspace<dim>::
NormalSubspace(const std::vector<bitvector::BitVector<dim> >& b, size_t n)
  :d_rank(n)

/*
  Synopsis: constructs the normal subspace generated by b.
*/

{
  using namespace bitvector;

  std::vector<bitvector::BitVector<dim> > b1 = b;
  normalize(d_support,b1);

  d_basis.swap(b1);
}

/******** accessors **********************************************************/

template<size_t dim> 
void NormalSubspace<dim>::representative
  (bitvector::BitVector<dim>& r, const bitvector::BitVector<dim>& v) const

/*
  Synopsis: put in r the canonical representative of v modulo the subspace
*/

{
  using namespace bitvector;

  // get projection to subspace in d_basis
  BitVector<dim> pv = v;
  pv.slice(d_support);

  // go over to absolute terms
  BitVector<dim> w(v.size());
  combination(w,d_basis,pv.data());

  r = v;
  r += w;

  return;
}

/******** manipulators *******************************************************/

template<size_t dim>
void NormalSubspace<dim>::apply(const bitvector::BitMatrix<dim>& r)

/*
  Synopsis: applies r to the data.

  Precondition: r is a matrix of the appropriate size (it could go to a
  space of another dimension, even.)
*/

{
  using namespace bitvector;

  d_rank = r.numRows();

  std::vector<BitVector<dim> > b = d_basis;

  for (size_t j = 0; j < b.size(); ++j) {
    r.apply(b[j],b[j]);
  }

  NormalSubspace<dim> ns(b,d_rank);
  swap(ns);

  return;
}

template<size_t dim>
void NormalSubspace<dim>::swap(NormalSubspace& source)

{
  d_basis.swap(source.d_basis);
  d_support.swap(source.d_support);

  return;
}

}

/*****************************************************************************

        Chapter II -- The NormalSubquotient class

  ... explain here when it is stable ...

*****************************************************************************/

namespace subquotient {

template<size_t dim> NormalSubquotient<dim>::
NormalSubquotient(const std::vector<bitvector::BitVector<dim> >& b1, 
		  const std::vector<bitvector::BitVector<dim> >& b2, size_t n)
  :d_space(b1,n),
  d_subspace(b2,n)
  
/*
  Synopsis: constructs the subquotient where b1 generates the space, b2
  the subspace.
*/

{  
  // d_support holds the positions corresponding to the basis elements of
  // space that are the canonical basis of the subquotient section
  d_support = d_space.support();
  d_support &= ~d_subspace.support();
  d_support.slice(d_space.support());

  assert(d_space.support().contains(d_subspace.support()));
}

/******** manipulators *******************************************************/

template<size_t dim>
void NormalSubquotient<dim>::apply(const bitvector::BitMatrix<dim>& r)

/*
  Synopsis: applies r to the data.
*/

{
  d_space.apply(r);
  d_subspace.apply(r);

  d_support = d_space.support();
  d_support &= ~d_subspace.support();
  d_support.slice(d_space.support());

  return;
}

template<size_t dim>
void NormalSubquotient<dim>::swap(NormalSubquotient& source)

{
  d_space.swap(source.d_space);
  d_subspace.swap(source.d_subspace);
  d_support.swap(source.d_support);

  return;
}

}
  
/*****************************************************************************

        Chapter III -- Functions declared in subquotient.h

  ... explain here when it is stable ...

*****************************************************************************/

namespace subquotient {

template<size_t dim>
  void subquotientMap(bitvector::BitMatrix<dim>& msq, 
		      const NormalSubquotient<dim>& source,
		      const NormalSubquotient<dim>& dest,
		      const bitvector::BitMatrix<dim>& m)

/*
  Synopsis: puts in msq the matrix of the map induced by m at the subquotient
  level.

  Precondition: m has the correct size, in terms of source.rank() and 
  dest.rank(); it takes the space and subspace from source to those from dest;

  Algorithm: take the image trough m of the subquotient basis in source;
  project them onto the subquotient basis in dest.

  The subquotient matrix is expressed in terms of the canonical basis for the
  subquotient (which is made up of those elements of the basis of space that
  are _not_ in the support of subspace.)
*/

{
  using namespace latticetypes;

  msq.resize(dest.dimension(),0);

  // restrict m to source.space()
  for (size_t j = 0; j < source.space().dimension(); ++j)
    if (source.support().test(j)) {
      Component v;
      m.apply(v,source.space().basis(j));
      // go to canonical representative
      dest.representative(v,v);
      // get coordinates in canonical basis
      v.slice(dest.support());
      msq.addColumn(v);
    }

  return;
}

}

}
