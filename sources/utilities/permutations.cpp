/*!
\file
  This is permutations.cpp.  This file contains the non-template
  definitions of the functions declared in permutations.h

*/
/*
  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups

  For license information see the LICENSE file
*/

#include "permutations.h"

#include "constants.h"
#include "bitset.h"
#include "bitmap.h"
#include "matrix.h"

/*
  This file contains the non-template definitions of the functions declared
  in permutations.h
*/

namespace atlas {

namespace permutations {


Permutation::Permutation(unsigned long n, int) // identity
  : Base(n)
{
  for (size_t i=n; i-->0; ) Base::operator[](i)=i;
}

Permutation::Permutation(const Permutation& pi, int) // inverse
  : Base(pi.size())
{
  for (size_t i=size(); i-->0; ) Base::operator[](pi[i])=i;
}



template<size_t n>
  bitset::BitSet<n> Permutation::pull_back(const bitset::BitSet<n>& v) const
{
  assert(size()<=n);

  const Permutation& pi=*this;
  bitset::BitSet<n> result;
  for (size_t i=0; i<size(); ++i)
    result.set(i,v[pi[i]]);

  return result;
}

// Replace each index |i| in |v| by |pi[i]|, where |pi| is our permutation
template<typename U>
std::vector<U> Permutation::renumbering(const std::vector<U>& v) const
{
  const Permutation& pi=*this;
  std::vector<U> result; result.reserve(v.size());
  for (size_t i=0; i<v.size(); ++i)
    result.push_back(pi[v[i]]);
  return result;
}

/*!
  Synopsis: a *= b;

  Precondition : a holds a permutation of [0,N[; b holds a permutation of
  [0,M[; M + n <= N;

  Postcondition : a holds the permutation a_new of [0,N[ where a_new[i+n]
  = a[ b[i] + n ] for i in [0,M[; a is not changed outside the range
  [n,M+n[.

  NOTE : although we know that the permutation can be done in place with
  the aid of just a bitmap, we do the lazy approach here and make a copy
  of the range involved.
*/
void compose(Permutation& a, const Permutation& b, unsigned long n)
{
  std::vector<unsigned long> c(a.begin() + n, a.begin() + n + b.size());

  for (size_t j = 0; j < b.size(); ++j) {
    a[j+n] = c[b[j]];
  }
}

bitmap::BitMap Permutation::renumbering(const bitmap::BitMap& b) const
{
  bitmap::BitMap result(size());
  for (bitmap::BitMap::iterator it=b.begin(); it(); ++it)
    result.insert((*this)[*it]);

  return result;
}

/* Here we are again applying the permutation |p| to each of the entries
   of |v|, but the exceptional value of |except| is passed unchanged */
template<typename U>
std::vector<U>
   Permutation::renumbering(const std::vector<U>& v, U except) const
{
  const Permutation& pi=*this;
  std::vector<U> result; result.reserve(v.size());
  for (size_t i=0; i<v.size(); ++i)
    result.push_back(v[i]==except ? except : pi[v[i]]);
  return result;
}

// Replace each index |i| in |v| by |(*this)[i]|
template<typename U>
void Permutation::left_mult(std::vector<U>& v) const
{
  const Permutation& pi=*this;
  for (size_t i=0; i<v.size(); ++i)
    v[i]=pi[v[i]];
}

/*!
  Applies our permutation |pi| to the vector |v|. In other words, we send each
  entry v[i] to the new position v[pi[i]]; this means that afterwards for all
  |i|: |new_v[pi[i]]==old_v[i]|, or equivalently $new_v[i]=old_v[pi^{-1}[i]]$.
  This notion of permuting an arbitrary sequence is a left action of $S_n$.

  This is pulling back (right multiplication) through the inverse permutation.

  We are able to perform this permutation essentially in-place, using an
  auxiliary bitmap.
*/
template<typename T> void Permutation::permute(std::vector<T>& v) const
{
  assert(v.size()>=size());
  const Permutation& pi=*this;
  bitmap::BitMap seen(v.size()); // initialized empty

  for (unsigned long i = 0; i < v.size(); ++i)
    if (not seen.isMember(i))
    {
      seen.insert(i);
      for (unsigned long j=pi[i]; j!=i; j=pi[j]) // cycle from |pi[i]| to |i|
      {
	std::swap(v[i],v[j]); // transpose |v[i]] with other members in order
	seen.insert(j);
      }
    }
}

/*! Permutes rows and columns of the matrix according to the permutation,
  resulting in the matrix of the same operator, expressed in the permuted
  basis e_{a^{-1}[0]}, ... , e_{a^{-1}[n-1]}. This amounts to conjugating by
  the permutation matrix (delta_{a[j],j})_{i,j} that transforms from
  coordinates on the standard basis to those on that basis $(e_{a^{-1}_i})_i$

  Precondition: |m| is an |n| by |n|, and |a| a permutation of |n|

  Method: the old entry at (i,j) is moved to its new location (a[i],a[j]), in
  a separate copy (without trying to do the permutation of entries in place)
  */
template<typename T>
void Permutation::conjugate(matrix::Matrix_base<T>& M) const
{
  size_t n=size();
  assert (M.numRows()==n);
  assert (M.numColumns()==n);
  matrix::Matrix<T> result(n,n);
  const Permutation& pi=*this;
  for (size_t i=0; i<n; ++i)
    for (size_t j=0; j<n; ++j)
      result(pi[i],pi[j]) = M(i,j);

  result.swap(M); // export result in |M|
}

/*! Permutes rows and columns of the matrix according to the inverse
  permutation, resulting in the matrix of the same operator but expressed in
  the inverse-permuted basis e_{a[0]}, ... , e_{a[n-1]}. This amounts to
  conjugating by the inverse permutation matrix (delta_{i,a[i]})_{i,j} that
  transforms from coordinates on the standard basis to those on tha basis.

  Precondition: |m| is an |n| by |n|, and |a| a permutation of |n|

  Method: the new entry at (i,j) is set to the old entry at (a[i],a[j]), in a
  separate copy (without trying to do the permutation of entries in place)
*/
template<typename T>
void Permutation::inv_conjugate(matrix::Matrix_base<T>& M) const
{
  size_t n=size();
  assert (M.numRows()==n);
  assert (M.numColumns()==n);
  matrix::Matrix<T> result(n,n);
  const Permutation& pi=*this;
  for (size_t i=0; i<n; ++i)
    for (size_t j=0; j<n; ++j)
      result(i,j) = M(pi[i],pi[j]);

  result.swap(M); // export result in |M|
}


// Standardization is a method of associating to a sequence of numbers |a| a
// permutation |pi|, such that |a[i]<a[j]| implies |pi[i]<pi[j], and
// |a[i]<a[j]| implies that |pi[i]<pi[j] is equivalent to |i<j|. Equivalently,
// setteing |a=standardize(a).pull_back(a)| amounts to stable sorting of |a|.

template <typename U>// unsigned type
Permutation standardize(const std::vector<U>& a, size_t bound,
			std::vector<unsigned int>* stops)
{
  std::vector<unsigned int> count(bound,0);
  for (size_t i=a.size(); i-->0; ) // downwards might be faster
  {
    assert(a[i]<bound);
    ++count[a[i]];
  }

  U sum=0;
  for (size_t i=0; i<bound; ++i) // cumulate
  {
    size_t ci=count[i]; count[i]=sum; sum+=ci;
  }
  // now |count[v]| holds number of values less than |v| in |a|
  if (stops!=NULL)
  { stops->reserve(bound+1);
    stops->assign(count.begin(),count.end());
    stops->push_back(sum);
  }

  Permutation result(a.size());
  for (size_t i=0; i<a.size(); ++i )
    result[i] = count[a[i]]++;

  return result;
}



// Instantiation of templates (only these are generated)

template bitset::BitSet<constants::RANK_MAX>
Permutation::pull_back(const bitset::BitSet<constants::RANK_MAX>& v) const;
  // cartan_io, realform_io

template std::vector<unsigned int>
Permutation::renumbering(const std::vector<unsigned int>& v) const; // blocks

template std::vector<unsigned short>
Permutation::renumbering(const std::vector<unsigned short>& v) const; // kgb

template void
Permutation::left_mult(std::vector<unsigned short>& v) const; // complexredgp

template void
Permutation::left_mult(std::vector<unsigned long>& v) const; // rootdata,weyl,.

template void
Permutation::inv_conjugate(matrix::Matrix_base<int>& M) const; // weyl

template Permutation
standardize(const std::vector<unsigned int>& a, size_t bound,
	    std::vector<unsigned int>* stops);

} // |namespace permutations|

} // |namespace atlas|
