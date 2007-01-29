% Copyright (C) 2006 Marc van Leeuwen
% This file is part of the Atlas of Reductive Lie Groups software (the Atlas)

% This program is made available under the terms stated in the GNU
% General Public License (GPL), see http://www.gnu.org/licences/licence.html

% The Atlas is free software; you can redistribute it and/or modify
% it under the terms of the GNU General Public License as published by
% the Free Software Foundation; either version 2 of the License, or
% (at your option) any later version.

% The Atlas is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
% GNU General Public License for more details.

% You should have received a copy of the GNU General Public License
% along with the Atlas; if not, write to the Free Software
% Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


\def\emph#1{{\it #1\/}}
\let\cong=\equiv

@* Introduction.
%
The purpose of this program is to read the files binary written by the
\.{atlas} software for saving the results of computations of Kazhdan-Lusztig
polynomials, or produced from them by the \.{matrix-merge} and
\.{coef-merge} programs, and to give the use access to these results in a
human-readable form.

@h <string>
@h <vector>

@c
@< Type definitions @>
@< Constant definitions @>
@< Global variable definitions @>
@< Function declarations @>
@< Function definitions @>
@< Main program @>


@ We shall use a global variable to controls whether constructing the internal
data structures of this program will produce any output to |std::cout| or
|std::cerr|. Normally constructors should bother writing output, but some of
the data structures considered here may take a lot of time to construct, and
it is informative for the user to know which one is taking the most time;
therefore this flag is set by default.

@< Global variable definitions @>=
bool verbose=true;


@ Let us begin with something elementary, reading a sequence of bytes from a
file into an integer. We will have to do this for various numbers of bytes at
once: 4~bytes for numbers that represent block elements or indices of
polynomials, 5~bytes for numbers that point in the sequence of polynomial
coefficients (which for split~$E_8$ has more than $1.3*10^{10}$ elements of up
to $4$ bytes each), and a number varying from~$1$ to~$4$ for those
coefficients themselves. To return the result, we need a type that can hold
more than 32~bits; it would seem that |unsigned long long int| is intended for
this purpose, so we shall use that.
@< Type definitions @>=

typedef unsigned long long int ullong; // sufficiently long unsigned type

@~To allow for maximal efficiency (since this function is going to be called
very often) we make the number of bytes a template parameter, and declare the
function inline, so that the compiler can roll out the dependence on~$n$
completely. It is then natural to make the template function recursive on its
template argument, and to provide an explicit specialisation for one byte to
terminate the recursion. Note that since we are using little-endian byte
order, recursion is in any case the easiest way to assure increasing left
shift amounts for each following byte.

This does mean that for reading polynomial coefficients, where the number of
bytes is determined at runtime, we need an additional function
|read_var_bytes| that dynamically selects one of finitely many instances of
the template function. We could limit ourselves to instances for up to~$4$
bytes, since no cases requiring more bytes are envisioned (and even the
$4$-byte case is only useful for historic reasons: while for split~$E_8$ in
fact three bytes suffice, this was only proved after completing the $4$-bytes
case without finding any large coefficients, and since none of the previous
$3$-byte runs had produced usable output, the $4$-byte file was for some time
the only one available; it even is as this text is being written). However, it
costs very little to include a few more cases, so we go up to the $8$ bytes
that (probably) exhaust the capacity of the |unsigned long long int| return
value.

@h <iostream>
@h <stdexcept>
@< Function definitions @>=
template <unsigned int n>
inline ullong read_bytes(std::istream& in)
{@;
  return static_cast<unsigned char>(in.get())+(read_bytes<@[n-1@]>(in)<<8);
}

template<>
inline ullong read_bytes<1>(std::istream& in)
{@;
  return static_cast<unsigned char>(in.get());
}

ullong read_var_bytes(unsigned int n,std::istream& in)
{ switch(n)
  { case 1: return read_bytes<1>(in);
    case 2: return read_bytes<2>(in);
    case 3: return read_bytes<3>(in);
    case 4: return read_bytes<4>(in);
    case 5: return read_bytes<5>(in);
    case 6: return read_bytes<6>(in);
    case 7: return read_bytes<7>(in);
    case 8: return read_bytes<8>(in);
    default: throw std::runtime_error("Illegal read_var_bytes");
  }
}

@ At one point we shall also have the occasion to write a number to a binary
file. This is always a $4$-byte number so we define just this special case,
using little-endian notation as always.

@< Function definitions @>=
void write_int(unsigned int n, std::ostream& out)
{ out.put(char(n&0xFF)); n>>=8;
@/out.put(char(n&0xFF)); n>>=8;
@/out.put(char(n&0xFF)); n>>=8;
@/out.put(char(n));
}


@* The coefficient file.
%
Now we come to the files with information about the Kazhdan-Lusztig
polynomials or other data needed to access them. In each case we shall define
a class that holds information read from the file, and in case of large files
also a reference to the file stream itself, so that it can look up information
requested when needed.

The simplest type of file is that for the coefficients, and we shall deal with
that first, even though the class was the last one to be defined since we
originally proceeded without using a class at all. The file has a very simple
format: it starts with a $4$-byte indication of the number~$n$ of polynomials
(we maybe unwisely decided that since split~$E_8$ has fewer that $2^{32}$
polynomials, we may assume that $n$ always fits in $4$-bytes), then a sequence
of $n+1$ indices of $5$~bytes each, which record where each of the
polynomials' coefficients start (relative to the start of the sequence of
coefficients), and finally a long sequence of those coefficients.

There is not very much information we need to store permanently about this
file. Apart from the reference to the file stream, we record the number of
polynomials, the size in bytes and the number of the coefficients, and the
offsets in the file where the index and coefficient parts start, respectively.
We also store a value that limits the length of the polynomials that we expect
to find, since this is quite an effective way to detect file corruption before
it does too much harm. This value will be $32$ for split~$E_8$, but we allow
it to be set in the constructor for this class, so that this value is not
hard-wired into the program.

@h <fstream>
@< Type definitions @>=

typedef ullong KLIndex;
 // must be |long long| to avoid problems when multiplied

class polynomial_info
{ std::ifstream& file; // non-owned reference

  KLIndex n_pols;  // must be 64-bits, for computing |5*n_pols|
  unsigned int coef_size;
  ullong n_coef;
  std::streamoff index_begin, coefficients_begin;
@)
  const size_t max_length;
public:
  polynomial_info(std::ifstream& coefficient_file, size_t deg_limit);
@)
  KLIndex n_polynomials() const @+{@; return n_pols; }
  unsigned int coefficient_size() const @+{@; return coef_size; }
  ullong n_coefficients() const @+{@; return n_coef; }
@)
  std::vector<ullong> coefficients(KLIndex i) const;
};

@ In the constructor we can set certain informations in the initialisation
part, but not all because we are limited to setting them in the order
declared, and not perform any statements in between. So we just get |n_pols|
by reading the very first bytes of the file, and then storing the file offset
reached in |index_begin|; we also copy the degree limit given as argument
into~|max_length|; the degrees of polynomials encountered are expected to be
strictly smaller, and there length therefore weakly smaller, than this number.

The coefficient size is not actually stored explicitly in the file, but we can
use the fact that polynomial number~$1$ is always the constant~$1$ for which
exactly one coefficient is stored, and the length of the sequence of
coefficient for this polynomials is therefore equal to the coefficient size
(no coefficients are stored for the null polynomial number~$0$, so we cannot
used that one). The length of that sequence is the difference between index
number~$2$ and index number~$1$, and since the latter is equal to the length
of all preceding coefficients, which is~$0$, we can just read the $5$-bytes
obtained after skipping the $10$ (null) bytes of the first two indices.

@< Function definitions @>=

polynomial_info::polynomial_info
  (std::ifstream& coefficient_file, size_t deg_limit)
: file(coefficient_file), n_pols(read_bytes<4>(file))
, coef_size(), index_begin(file.tellg()), coefficients_begin()
, max_length(deg_limit)
{ file.seekg(10,std::ios_base::cur); // skip initial 2 indices
  coef_size=read_bytes<5>(file); // size of the |One|
  file.seekg(index_begin+5*n_pols,std::ios_base::beg);
  n_coef=read_bytes<5>(file)/coef_size;
  coefficients_begin=file.tellg();
}

@ The main accessor function is the one that extracts a sequence of polynomial
coefficients from the file. Their number is determined by the difference
between two indices, divided by the coefficient size. If this number exceeds
|max_length| we throw a |runtime_error| after indicating the poly index and
degree in question. Otherwise we position the file for reading the
coefficients, and read them. Afterwords we test is the file was large enough
to allow reading, which is particularly relevant when we are so impatient that
we run \.{KLread} before the file is completely written (for split~$E_8$ this
can take as much as days, depending on the hardware used). If there was a
problem reading the file we throw a |runtime_error|, but we clear the error
status of the file first so that the user can try again with a more modest
index.

@< Function definitions @>=
std::vector<ullong> polynomial_info::coefficients(KLIndex i) const
{ file.seekg(index_begin+5*i,std::ios_base::beg);
  ullong index=read_bytes<5>(file);
  ullong next_index=read_bytes<5>(file);
  size_t length=(next_index-index)/coef_size;
  if (length>max_length)
    {
      std::cerr << "Found degree " << length-1 @|
		<< ", for polynomial #" << i << ".\n";
      throw std::runtime_error("Degree exceeds limit given");
    }

  std::vector<ullong> result(length);
  file.seekg(coefficients_begin+index,std::ios_base::beg);

  for (size_t i=0; i<length; ++i)
    result[i]=read_var_bytes(coef_size,file);

  if (not file.good())
  @/{@;
      file.clear();
      throw std::runtime_error("Input error reading coefficients");
    }

  return result;
}

@* The block data.
%
Next we shall consider the file with block data. It is not much more
complicated than the one with polynomials, but we reconstruct from it some
tables that will be necessary for converting pairs of indices into the matrix
into ``primitive'' ones, and these will be stored together with the other data
in the |block_info| structure (due to the fairly large number of fields that
will be used by others, we shall in this case not bother to define a |class|
with controlled access to these fields).

The information stored about a block comes in three groups: some global
information such as the block size and the rank, followed by a description of
the ``descent sets'' of all block elements (given as bitmaps indexed by simple
roots), and finally ``ascent tables'' for those elements, telling how they can
possibly be raised to another block element according to some simple root.


We start by introducing some types that will be used to store these data, and
other data derived from it. Block elements themselves will be stored as
|unsigned int| values, which seems largely sufficient. The descent sets will
be represented by values of type |RankFlags|, which is chosen somewhat large
with respect to our current computational capabilities: it allows the rank to
be up to~$32$. Combining these for all block elements we get a
|descent_set_vector|. For the ascent tables we need more space, since for each
block element and simple root we potentially need to store a block element. We
define an |ascent_table| as vectors of vectors, to be subscripted at the outer
level by the block element, and at the inner level by a simple root (this
gives more storage overhead than the opposite order of selection would, but
for primitivisation it is more convenient to fix an |ascent_vector| while
searching for a suitable simple root). The ascent tables implicitly define the
notion of (weakly) primitive elements for a given descent set, which are the
elements that cannot be raised for the roots in that set. For each descent set
we shall compute a |prim_list| containing those primitive elements, and
together these will be grouped into a~|prim_table|.

@h <bitset>
@< Type definitions @>=

typedef unsigned int BlockElt;
typedef std::bitset<32> RankFlags; // we can go up to rank 32
typedef std::vector<RankFlags> descent_set_vector; // indexed by block element
@)
typedef std::vector<BlockElt> ascent_vector;       // indexed by simple root
typedef std::vector<ascent_vector> ascent_table;   // indexed by block element
@)
typedef std::vector<BlockElt> prim_list;           // list of weak primitives
typedef std::vector<prim_list> prim_table;
 // effectively indexed by |RankFlags|

@ Here is the class holding the information about a block as read from a file.
Since all the information is read in during construction we do not store a
reference to the file stream. After each major variable we give estimates of
memory consumption the represent for split~$E_8$, on a $32$-bit machine, based
on average of $15000$ (weakly) primitive elements per descent set. These
estimates show that memory consumption should not prohibit running this
program on such an architecture. (One could not that for the empty descent set
all block elements will be weakly primitive, and that this contribution of
$453060$ can throw some doubt on the estimate of the average which was in fact
roughly calculated after the fact from an average over the rows (rather than
descent sets) of the number of \emph{strongly} primitive elements; if needed
we could exclude the empty set from storage in |prim_table| at the cost of
some extra code.

@< Type definitions @>=
struct block_info
{
  unsigned int rank;
  size_t size;
  unsigned int max_length;
  std::vector<BlockElt> start_length;
   // size |max_length+2|; defines intervals for each length
@)
  descent_set_vector descent_set;     // $453060*4    =  1812240$ bytes:  2~MB
  ascent_table ascents;               // $453060*8*4  = 14497920$ bytes: 13~MB

  prim_table primitives_list;         // $256*15000*4 = 15360000$ bytes: 14~MB
@)
  block_info(std::ifstream& in); // constructor reads file
@)
  BlockElt primitivise(BlockElt x, const RankFlags d) const;
private:
  bool is_primitive(BlockElt x, const RankFlags d) const;
  void compute_prim_table();
};

@ The table |ascents| will serve to implement the operation~|primitivise|. For
each pair of a block element~$x$ and a simple root~$s$, the table can give an
other element~$x'$ such that $x$ can be replaced by~$x'$ in the computation of
$P_{x,y}$ whenever $s$ is in the descent set of~$y$. For this to happen it is
necessary, but not sufficient, that $s$ is not in the descent set of~$x$; in
the cases where no replacement of~$x$ is possible for~$s$ (including those
where $s$ \emph{is} in the descent set of~$x$) a special value |noGoodAscent|
is indicated in the table. There is a third possibility, namely that $x$ is
not replaced by another element, but that one may conclude directly that
$P_{x,y}=0$ (this happens when the ascent is ``real non-parity''); in this
case another special value |UndefBlock| is stored in the table (a similarly
named value is used in the Atlas program; that program has no equivalent for
|noGoodAscent| however, since it can deduce this form other information, which
has however not been written to the file with block information). Since blocks
are not expected to come near to filling up all the possible values of the
|BlockElt| type, we reserve two values at the end of the range for these
special indicators. (Note that the \Cpp\ compiler understands that
|~BlockElt(0)| is not a call to a destructor, but a bitwise complement taken
of a cast to the (integral) type |BlockElt|; this cast is not strictly
necessary, but it makes clear that the complement is to be taken over the full
width of the integer.)

@< Constant definitions @>=

const BlockElt UndefBlock= ~BlockElt(0);
const BlockElt noGoodAscent= UndefBlock-1;

@ The method |primitivise| finds for a given block element~$x$ and the descent
set~$d$ of a block element~$y$ another element~$z$ such that
$P_{x,y}=P_{z,y}$ and which is (weakly) primitive for that descent set,
meaning that $ascent[z][s]=noGoodAscent$ for all $s\in{d}$. Alternatively it
may return |z==UndefBlock|, meaning that it has been established that
$P_{x,y}=0$ (in this case the precise entry in |ascent| that was found to be
|UndefBlock| is not revealed). The algorithm is straightforward: we repeatedly
apply the first good ascent that is found, or return |UndefBlock| whenever we
hit that. It is conceivable that it could be arranged to hit |UndefBlock|
through certain sequences of ascents but not via others, but to try to find
these sequences by backtracking would certainly not win time. On the other
hand it might be feasible to try do propagate any |UndefBlock| entries in the
table downward to entries that will lead to them, but this would be more
effective if the table were to be indexed directly by a descent set rather
than by a single simple root as we have preferred to do here, for reasons of
economy of memory.

@< Function definitions @>=
BlockElt block_info::primitivise(BlockElt x, const RankFlags d) const
{ start:
  const ascent_vector& ax=ascents[x];
  for (size_t s=0; s<rank; ++s)
    if (d[s] and ax[s]!=noGoodAscent)
      {
	if (ax[s]==UndefBlock) return UndefBlock;
	x=ax[s]; goto start; // this should raise x, now try another step
      }
  return x; // no raising possible, stop here
}

@ The auxiliary method |is_primitive| only uses the |ascents| member of the
block, which means it can be called from the |block_info| constructor as soon
as the |ascents| table is properly initialised; it will in fact be used to
construct the |primitives_list| table. The value it returns can be expressed
as |x==primitivise(x,d)|, but it is more efficiently computed in case this
is~|false|.

@< Function definitions @>=

bool  block_info::is_primitive(BlockElt x, const RankFlags d) const
{
  const ascent_vector& ax=ascents[x];
  for (size_t s=0; s<ax.size(); ++s)
    if (d[s] and ax[s]!=noGoodAscent) return false;
  return true;
  // now |d[s]| implies |ascents[s]==noGoodAscent| for all simple roots |s|
}

@ The method |is_primitive| will be repeatedly called by another auxiliary
method |compute_prim_table|, which also can be called as soon as |ascents| is
initialised; for each descent set it traverses all block elements, collecting
into the vector |primitives_list[s]| the ones that satisfy the condition
|is_primitive|. We do not know beforehand how many elements this will give,
but at the end we do not need any spare capacity, so we reallocate the final
vector to one of the precise capacity needed: we copy-construct an anonymous
duplicate (which will have no spare capacity), and then swap its contents with
the original vector.

@< Function definitions @>=

void block_info::compute_prim_table()
{
  size_t powerset_rank= 1ul<<rank;
  primitives_list.resize(powerset_rank);
    // fills |primitives_list| with empty vectors
  for (size_t s=0; s<powerset_rank; ++s)
    { prim_list& this_list=primitives_list[s];
@)
      if (verbose)
	std::cerr << "Constructing weakly primitive elements for descent set "
		  << s << '\r';
@)
      RankFlags d(s);
      for (BlockElt x=0; x<size; ++x)
	if (is_primitive(x,d)) this_list.push_back(x);
      prim_list(this_list).swap(this_list); // reallocate to fit snugly
    }
@)
    if (verbose)
      std::cerr << "\nDone constructing weakly primitive elements.\n";
}

@ The constructor for a block basically just reads all of the block file, and
places the information read into the proper variables. This function can
almost be considered as a specification of the block file format. Note that
only |max_length| values of length information are stored in the file,
recording the internal boundaries between intervals of block elements of equal
length; the initial bound~$0$ for the first interval and the final
bound~|size| of the final interval are provided here without reading anything
from the file.

@< Function definitions @>=

block_info::block_info(std::ifstream& in)
  : rank(), size(), max_length(), start_length()
  , descent_set(), ascents(), primitives_list() // don't initialise yet
{
  in.seekg(0,std::ios_base::beg); // ensure we are reading from the start
  size=read_bytes<4>(in);
  rank=read_bytes<1>(in);
  max_length=read_bytes<1>(in);
@)
  if (verbose)
    std::cout << "rank=" << rank << ", block size=" << size @|
	      << ", maximal length=" << max_length << ".\n";
@)
  // read intervals of block elements for each length
  start_length.resize(max_length+2);
  start_length[0]=BlockElt(0);
  for (size_t i=1; i<=max_length; ++i) start_length[i]=read_bytes<4>(in);
  start_length[max_length+1]=size;
@)
  // read descent sets
  descent_set.reserve(size);
  for (BlockElt y=0; y<size; ++y)
    descent_set.push_back(RankFlags(read_bytes<4>(in)));
@)
  // read ascent table
  ascents.reserve(size);
  for (BlockElt x=0; x<size; ++x)
    {
      ascents.push_back(ascent_vector());
      ascent_vector& a=ascents.back();
      a.reserve(rank);
      for (size_t s=0; s<rank; ++s)
	a.push_back(read_bytes<4>(in));
    }
@)
  compute_prim_table();
}


@* The matrix file.
%
The final type of binary file interpreted by this program is the one
containing the matrix information. What it represents is quite simple: a huge
triangular matrix with $32$-bit integral coefficients, which will be
interpreted as indices of polynomials stored in the polynomial file. Several
methods are employed however to limit the size of the file, which makes this
file type the most complicated to handle; also because of the size, no attempt
is made to read in all information at once, just as was the case for the
polynomial coefficient file.

The file consists of a simple sequence of parts for each ``row'' of the matrix
(actually one should say column, since it is the second index~$y$ that is
fixed); each part mentions the row number~$y$ (as a check), then the number of
weakly primitive elements for this row (also as a check, and to facilitate
interpreting the rest of the row data), then a bitmap showing which of those
primitive elements~$x$ have non-zero Kazhdan-Lusztig polynomial $P_{x,y}$ (in
this case we shall say $x$ is strongly primitive for~$y$), and finally a
sequence of indices of the Kazhdan-Lusztig polynomials for those strongly
primitive elements. The set of primitive elements for row~$y$ is easily
reconstructed from the block data alone: it is the initial part of the
|primitives_list| for |descent_set[y]| of those elements of length strictly
less than that of~$y$, followed as final element by~$y$ itself.

The bitmap allows extracting from this list the subset of strongly primitive
elements, but that depends more essentially on $y$ itself (rather than just on
|descent_set[y]|), so it is not practical to precompute the lists of strongly
primitive elements for all~$y$ at once, nor even to hold all the bitmaps in
memory; the program will read in the bitmap and extract the strongly primitive
elements only once a value of~$y$ is selected by the user. The vector holding
strongly primitive elements has the same data type as those holding weakly
primitive ones, but we provide a |typedef| for it to emphasise the intended
interpretation.

@< Type definitions @>=

typedef prim_list strong_prim_list;
 // strongly primitive elements for fixed |y|

@ Once a row~|y| is selected, we store the information for that row in the
|matrix_info| class, so that subsequent queries for the same row can be
handles rapidly. The main public method is |find_pol_nr|, which looks up the
index of $P_{x,y}$ and also tells the primitive value~$xx$ into which $x$ was
converted before looking up the index.

@< Type definitions @>=

class matrix_info
{
  std::fstream& matrix_file;

  block_info block;

  std::vector< unsigned int> n_weak_prims; // $453060*4= 1812240$ bytes: 2 MB
  std::vector<std::streampos> row_pos;     // $453060*8= 3624480$ bytes: 3 MB

/* We avoid storing:\hfill\break
|std::vector<std::vector<bool> > prim_map;|
 // 453060*1600  =724896000 bytes: 691 MB
\hfill\break*/
@)// data for currently selected row~|y|
  BlockElt cur_y;		       // $4$ bytes
  strong_prim_list cur_strong_prims;   // $6400*4 = 25600$ bytes (but max 2MB)
  std::streampos cur_row_entries;
   // 8 bytes; points to start of indices for |cur_strong_prims|
@)//private methods
  matrix_info(const matrix_info&); // copying forbidden
  void set_y(BlockElt y);          // install |cur_y| and dependent data
@)
public:
  enum mode @+{ old, revised, transform };
  matrix_info(std::ifstream* block_file,std::fstream* m_file, mode format);
  ~matrix_info() @+{@; delete &matrix_file; }
@)
  BlockElt block_size() const @+{@; return block.size; }
  KLIndex find_pol_nr(BlockElt x,BlockElt y,BlockElt& xx);
  // sets |y|, so not |const|
};


@ Before looking at the constructor, which has to scan the entire file to
locate the positions~|row_pos[y]| in the file for all~$y$, we look at the
method |set_y| that selects a row and prepares for reading from it. Apart from
|row_pos[y]|, the number |n_weak_prims[y]| of weak primitive elements for~$y$
has also been stored during construction, so after seeking to |row_pos[y]| we
can go right ahead reading the bitmap. The interpretation of all bit positions
except the final one depends only on |descent_set[y]|, namely the block
element recorded at the corresponding position of the vector |weak_prims|
defined below. The final bit is always set and corresponds to~|y|, which is
probably not the next element of |weak_prims| (in retrospective it would have
been better to exclude this useless bit from the matrix file, but it is not
easy to change the format now). However, |y| is always present in
|weak_prims|, so we run no risk in interpreting the bitmap on an initial part
of |weak_prims| first, and then replacing the final element by~|y|; just to be
sure, we do check that the element replaced is the one we expect it to be.

@< Function definitions @>=
void matrix_info::set_y(BlockElt y)
{
  if (y==cur_y) {@; matrix_file.seekg(cur_row_entries); return; }
  cur_y=y;
  const prim_list& weak_prims
    = block.primitives_list[block.descent_set[y].to_ulong()];
  cur_strong_prims.resize(0);
  // restart building from scratch, but don't deallocate storage

  unsigned int n_prim=n_weak_prims[y];
  matrix_file.seekg(row_pos[y]);
@)
  @< Read $\lceil$|n_prim/32|$\rceil$ bytes of bitmap data from |matrix_file|,
     and extract |cur_strong_prims| from an initial portion |weak_prims|
     according to it @>
  @< Check value of final element pushed, and replace it by~$y$ @>
@)
  cur_row_entries=matrix_file.tellg();
}

@ Extracting according to the bitmap is done straightforwardly by shifting all
bits successively in the hairline position (bit~$0$), and selecting a value
from |weak_prims| if the bit was set. There exist methods for traversing only
the set bits, but the relatively large effort needed to find out which bit was
found would not justify using them except for rather sparse bitmaps, which is
not what we expect here. Nevertheless we take some advantage of the
possibility of skipping the rest of a word when it no longer has any bits set,
which is why the inner loop below terminates when |chunk==0| rather than when
|j==32|.

@< Read $\lceil$... @>=
for (size_t i=0; i<n_prim; i+=32)
  {
    unsigned int chunk=read_bytes<4>(matrix_file);
    for (size_t j=0; chunk!=0; ++j,chunk>>=1) // and certainly |j<32|
      if ((chunk&1)!=0) cur_strong_prims.push_back(weak_prims[i+j]);
  }

@ The final element that we pushed ought to be the first element  in
|weak_prims| of the same length as~|y|. We can locate it quickly by doing
binary search for~|y| in the vector |max_length|, and then for the first block
element of the length of~|y| in |weak_prims|, in both cases without expecting
to find exactly the element specified. Since |y| might itself be the first
element of its length, we have to look to use |upper_bound| in the first
search, and |lower_bound| in the second one. If all is well, we replace the
element found by~|y|.

@h <cassert>
@< Check value of final element pushed, and replace it by~$y$ @>=
{
  const BlockElt* i= // point after first block element of length of |y|
    std::upper_bound(&block.start_length[0]
		    ,&block.start_length[block.max_length+1]
		    ,y);
  const BlockElt* first= // point to first of |weak_prims| of that length
    std::lower_bound(&weak_prims[0],&weak_prims[weak_prims.size()],*(i-1));
  assert(cur_strong_prims.back()==*first);
  cur_strong_prims.back()=y; // replace by |y|
}

@ The method |set_y| serves to facilitate the method |find_pol_nr| which looks
up the index of a polynomial in the matrix file. Since |set_y| leaves the read
pointer at the beginning of the sequence of such indices, all we need to do is
primitivise~|x| to~|xx|, look up~|xx| in |cur_strong_prims|, and either read
the index at the corresponding offset in the file, or return~|0| (the index of
the null polynomial) if |xx| was not found. In fact it is also possible that
|xx==UndefBlock|, in which case we can return~|0| immediately.

In the first argument of |matrix_file.seekg| below, it seems impossible that
|it-cur_strong_prims.begin()| could be interpreted as a negative number (it
would require that |cur_strong_prims| occupy more than half of the virtual
address space (on a $32$ bit machine), in which case almost certainly memory
would overflow elsewhere), but as a matter of principle we cast the pointer
difference to an unsigned type of the same size (|size_t|) to ensure that the
offset used is never widened to |std::streamoff| as a negative number.


@< Function definitions @>=
KLIndex matrix_info::find_pol_nr(BlockElt x,BlockElt y,BlockElt& xx)
{
  set_y(y);
  RankFlags d=block.descent_set[y];
  xx=block.primitivise(x,d);
  if (xx==UndefBlock) return KLIndex(0); // primitivisation copped out
  strong_prim_list::const_iterator it=
    std::lower_bound(cur_strong_prims.begin(),cur_strong_prims.end(),xx);
  if (it==cur_strong_prims.end() or *it!=xx) return KLIndex(0); // not strong

  matrix_file.seekg(4*size_t(it-cur_strong_prims.begin()),std::ios_base::cur);
  return KLIndex(read_bytes<4>(matrix_file));
}

@ The constructor for |matrix_info| must do quite a bit of work, but its
outline is quite simple. After storing the file reference (which will be
needed throughout the life of the object) and reading initialising the |block|
sub-object using the block file, we go through the parts of the matrix file in
a loop. Since we shall need to know the length of the block element~|y|
parametrising the current row, we keep a variable~|l| recording this, which is
possibly updated for each new~|y|using the |length_start| vector of the block.
The first $4$~bytes of each part should give the row number~|y|, but we allow
a special exemption for |y==0|, so that the very first bytes of a file can be
used to store a format identification.

The final parameter |format| of this constructor is there to solve an
inconvenience of the original file format, which make is laborious to find the
extent of individual parts of the matrix file, as we shall see below. To
alleviate this, a slightly modified file format is defined in which the
necessary information for an immediate advance to the next part is written
into the file. The constructor can be invoked assuming the original format,
with |format==old|, or assuming the new format, with |format==revised|, or
with s special value |format=transform|, in which case the format expected is
the old one, but the constructor will proceed to rewrite the file into the new
format.

@< Function definitions @>=
matrix_info::matrix_info
  (std::ifstream* block_file,std::fstream* m_file, mode format)
@/: matrix_file(*m_file) // store reference to the matrix file
  , block(*block_file) // read in block information
  , n_weak_prims(block.size)
  , row_pos(block.size) // dimension these vectors
  , cur_y(UndefBlock), cur_strong_prims(), cur_row_entries(0)
{

  if (verbose)
    std::cerr << "Starting to scan matrix file by 'rows'.\n";
@)
  size_t l=0; // length of y
  matrix_file.seekg(0,std::ios_base::beg);
  for (BlockElt y=0; y<block.size; ++y)
    { @< Advance |l| if necessary to obtain |block.start_length[l] <= y
         < block.start_length[l+1]| @>

      if (verbose and format!=revised) std::cerr << y << '\r';
@)
      if (read_bytes<4>(matrix_file)!=y and y!=0)
      @/{@; std::cerr << y << std::endl;
        throw std::runtime_error ("Alignment problem");
      }

      @< Determine |n_weak_prims[y]| and |row_pos[y]| @>
      @< Advance the read pointer to the beginning of the next row of the
         matrix file @>
      if (not matrix_file.good())
      @/{@; std::cerr << y << std::endl;
	  throw std::runtime_error ("Premature end of file");
	}
    }

    if (verbose) std::cerr << "\nDone scanning matrix file.\n";

  delete block_file; // success, we no longer need the block file
}

@ Since the block elements are stored by weakly increasing length, and
|start_length| gives the boundaries between different lengths, it is easy to
keep in |l| the length of~|y|. There are probably no holes in the sequence of
occurring lengths, but by using |while| rather than |if| we can cater for the
possibility without additional effort. when increasing~|l|, we have a nice
opportunity to give an informative progress report.

@< Advance |l| if necessary... @>=
while (y>=block.start_length[l+1])
{ ++l;
  if (verbose)
    std::cerr << "length " << l << " starts at y=" << y << std::endl;
}

@ The original format for the matrix file contains as first entry after the
row number the number of weakly primitive elements for this row. This is
useful for the \.{matrix-merge} programs, since it allows the following bitmap
to be interpreted without any knowledge about the block. Here however we do
have the block information at hand, and we can compute the expected number of
weakly primitive elements from it. This allows us on one hand to do a
consistency check, but also to replace this number by another one that will
speed up the construction of |matrix_info|, which is what is done for the
revised format (since the matrix files can be huge, we prefer to revise the
format in a way that does not change the file length and only overwrites small
pieces of the file). When |format==transform| we shall in fact expect the old
format, but write back the new format to the file. Therefore we perform the
consistency check here when the |format| is |old| or |transform|, but when
|format==revised| we just trust our prediction and store it in
|n_weak_prims[y]|. Note that in the former case the read pointer has advanced
past the (original) primitive count information, while the latter case it
stays before that (revised) information, and |row_pos[y]| must be adjusted
accordingly.

@< Determine |n_weak_prims[y]| and |row_pos[y]|... @>=
{
  unsigned int n_prim;

  @< Compute in |n_prim| is the expected number of weak primitive elements
     for |y| @>

  if (format!=revised and read_bytes<4>(matrix_file)!=n_prim)
    @/{@; std::cerr << y << std::endl;
      throw std::runtime_error ("Primitive count problem");
    }

  n_weak_prims[y]=n_prim;
  row_pos[y]= matrix_file.tellg(); // record position where bitmap starts
  if (format==revised) row_pos[y]+=4;
   // which is still 1 tetra-byte ahead if |format==revised|
}

@ Predicting the number of weakly primitive elements is easy: we just count
the elements in the appropriate |prim_list| selected from the table
|primitives_list|, of length strictly less than~|y| (which can be done by a
binary search for the first element of the same length as~|y|), and add~$1$
for the element~|y| itself, which is considered weakly primitive for its own
row.

@< Compute in |n_prim| is the expected number of weak primitive elements
   for |y| @>=
  {
    const prim_list& weak_prims
      = block.primitives_list[block.descent_set[y].to_ulong()];
    prim_list::const_iterator i= // find limit of |length<l| values
      std::lower_bound(weak_prims.begin(),weak_prims.end()
		      ,block.start_length[l]);

    n_prim=size_t(i-weak_prims.begin())+1;
    // number of |weak_prims| of length~$<l$, plus~$1$ for~|y|
  }

@ Now comes the part where the different formats come into play. If
|format==revised| we are still before the revised primitive element count. The
revision amounts to replacing the value of weakly primitive elements by the
number of strongly primitive elements (for which the matrix file actually
records a polynomial index); we read it in, then advance over a number of
tetra-bytes equal to the number $\lceil$|n_weak_prims[y]/32|$\rceil$ of
$32$-bit groups in the bitmap plus the number of strongly primitive elements
just read.

When |format==transform| we need to compute |n_strong_prim| the hard way, but
then we shall back up to overwrite the old count by this newly computed one
for the new format. We save the location where this should be done in
|prim_count_pos| even when |format==old|, which should not be too much of a
waste.

@< Advance the read pointer... @>=
{ std::streampos prim_count_pos=matrix_file.tellg()+std::streamoff(-4);
@)
  size_t n_strong_prim;
  if (format==revised)
  { n_strong_prim=read_bytes<4>(matrix_file);
    matrix_file.seekg(4*std::streamoff((n_weak_prims[y]+31)/32+n_strong_prim)
		     ,std::ios_base::cur);
  }
  else
  { @< Set |n_strong_prim| to the sum of the bits in the bitmap @>
@)
    if (format==transform)
    { std::streampos here=matrix_file.tellg();
      matrix_file.seekg(prim_count_pos); // back up
      write_int(n_strong_prim,matrix_file);
      matrix_file.seekg(here);
    }
    matrix_file.seekg(4*std::streamoff(n_strong_prim),std::ios_base::cur);

  }
}


@ To find the offset to advance in a file past the part for row~|y|, we need
to find the number of set bits in its bitmap; since the bitmaps are quite
large, this is the point where the old format costs a lot of time. As basic
operation, we shall use a function~|add_bits| that returns the sum all the
bits in the word~|x| (sideways addition of bits), which is the number of its
set bits. Its argument type is |unsigned long int|, which we shall suppose is
the largest unsigned type that can be held in a single register, and on which
multiplication can be performed in hardware.

@< Function declarations @>=
unsigned int add_bits(unsigned long int x);

@~The following procedure is attributed by D.~E. Knuth to D.~B. Gillies and
J.~C.~P. Miller (pre-fascicle~1a to volume~4 of {\it The Art of Computer
Programming}, p.~11). The idea is to successively replace groups of 2, 4, and
8 bits by their sideways sum, and then to use a judicious multiplication and
extraction to add all the bytes (the multiplication and extraction could be
replaced by a reduction modulo~$255$, but we assume that multiplication is
faster than Euclidean division). This procedure works correctly whether an
|unsigned long int| has 32 or 64 bits (in fact it works for any multiple of
8~bits strictly less than~$2^8=256$, i.e., for which the sideways sum itself
can always be represented in 8~bits).

@< Function definitions @>=

unsigned int add_bits(unsigned long int x)
{ static const unsigned long int b0= ~(~0ul/3);
  // |0xAAAA|\dots ; flags odd bit positions
  static const unsigned long int b1= ~(~0ul/5);
  // |0xCCCC|\dots ; flags positions $\cong2,3 \pmod4$
  static const unsigned long int b2= ~(~0ul/17);
  // |0xF0F0|\dots ; flags positions $\cong4$--$7\pmod8$
  static const unsigned long int ones= ~0ul/255;
  // |0x0101|\dots ; flags the low bit of each octet
  static const unsigned int high_byte_shift=8*(sizeof(unsigned long int)-1);
@)
  x-=(x&b0)>>1;          // replace pairs of bits $10\to01$ and $11\to10$
  x=(x&~b1)+((x&b1)>>2);
   // sideways add 2 groups of pairs of bits to 4-tuples of bits
  x += x>>4;
   // the sums of octets (bytes) are now in lower 4-tuples of those octets
  return (x&~b2)*ones >> high_byte_shift;
   // add lower 4-tuples of bytes in high octet, and extract
}

@ We are now ready to rapidly add up the bits of the bitset. If our
architecture is $64$-bits (or more, who knows?), we wish to invoke |add_bits|
for that many bits at a time, but the bitmap size is rounded to a multiple of
$32$~bits. Therefore we start collecting as many groups as possible that fit
into an |unsigned long int|, and finish by collecting any remainder in
$32$-bit groups. For the first group we round down, for the second group up,
which implies that it might happen that we could have done the entire last
group with one more |unsigned long int| in the first group; the possible gain
in speed one could obtain by a different calculation vanishes however due to
the more tricky arithmetic that would be needed. By the way, one could remark
that we could have added up the bits by a per-byte look-up in a variant of the
function |read_bytes|, and the whole issue of efficiently summing of bits
would disappear; it was however much more fun to do it this way.

@< Set |n_strong_prim| to the sum of the bits in the bitmap @>=
{ static const unsigned int ulsize=sizeof(@[unsigned long int@]);
@)
  size_t count=n_weak_prims[y]/(8*ulsize);
  // number of |unsigned long int|s to read

  n_strong_prim=0;
  while (count-->0)
    n_strong_prim+=add_bits(read_var_bytes(ulsize,matrix_file));

  count=(n_weak_prims[y]%(8*ulsize)+31)/32;
   // maybe some tetra-byte(s) left to read
  while (count-->0)
    n_strong_prim+=add_bits(read_bytes<4>(matrix_file));
}

@* The main program.
%
Finally we are going to wrap everything up in the main program. It proceeds in
a fairly simple manner, processing the optional flags first, then the other
program arguments, constructing a |polynomial_info| object in all cases and a
|matrix_info| object if the number of arguments indicates that a block and
matrix file are present. The calling sequence is (with square brackets
indicating optional parts)
$$
 \hbox{|program_name| [\.{-q}] [\.{-l} |degree_limit|]
  [|block_file| |matrix_file|] [|coef_file|]}
$$
Here the option \.{-q} means set |verbose=false|, and \.{-l} |degree_limit|
means set the limit of the degrees of the polynomials to |degree_limit| (all
polynomials are expected to be of degree strictly less, providing a check
for data integrity; the default value~$32$ is adapted to split~$E_8$).

@h <sstream>
@h <cctype>
@< Main program @>=

int main(int argc,char** argv)
{ std::string program_name(*argv);
  --argc; ++argv; // read and skip program name
@)
  if (argc>0 and std::string(*argv)=="-q") {@; verbose=false; --argc; ++argv;}
@)
  size_t degree_limit=32; //default value is OK for split~$E_8$
  if (argc>=2 and std::string(*argv)=="-l")
    // then override default |degree_limit|
  { std::istringstream arg_text(argv[1]);
    arg_text >> degree_limit;
    if (arg_text.good()) @+{@; argc-=2; argv+=2; }
    else @+{@; std::cerr << "Non-numeric argument following -l\n"; exit(1); }
  }
@)
  std::auto_ptr<matrix_info> mi; // auto-pointer guarantees clean-up at end
  std::ifstream coef_file;
@)
  @< Scan arguments and do initial processing of input files @>
@)

  while (true) @< Process use input, do |break| when done @>


}

@ We need to open files in binary mode (to avoid any conversion of end-of-line
codes that could be accidentally present); all files are opened for read only,
except the matrix file when a transformation is requested, which will be opened
for read/write.

@< Constant definitions @>=
const std::ios_base::openmode binary_in=
			    std::ios_base::in
			  | std::ios_base::binary;

const std::ios_base::openmode binary_in_out=
			    std::ios_base::in
			  | std::ios_base::out
			  | std::ios_base::binary;


@ Here we count the number of (remaining) arguments; when there are three,
then a block and matrix file are supposed to be present. Once the coefficient
file is successfully opened, we define a |polynomial_info| object using it;
this is the reason that the current module is not enclosed in braces. Once the
polynomial information is read, we print the number of polynomials and
coefficients for the user's enlightenment.

@< Scan arguments and do initial processing of input files @>=
if (argc>=3)
  @< Construct |matrix_info| object using first two command line arguments,
     and make |mi| point to it @>

if (argc==1) coef_file.open(argv[0],binary_in);
else
  {
    std::string file_name;
    std::cout << "File name: " ;
    std::cin >> file_name;
    coef_file.open(file_name.c_str(),binary_in);
  }
if (not coef_file.is_open())
  {@; std::cerr << "Open failed"; exit(1); }

polynomial_info pol(coef_file,degree_limit);

std::cout << "Coefficient size " << pol.coefficient_size() << ".\n" @|
          << pol.n_polynomials() << " polynomials, "
	  << pol.n_coefficients() << " coefficients.\n";

@ When a block and matrix file are given, we allocate the corresponding stream
object dynamically, because pointers to them will be held in the |matrix_info|
object. We first open both for reading, but if transformation is requested the
matrix file must be reopened for writing as well. Only in the case of errors
do we delete the pointers allocated here (which closes the files if
necessary).

@< Construct |matrix_info| object... @>=
{ argc-=2; // we will consume two arguments
  std::ifstream* block_file=new std::ifstream;
@/std::fstream* matrix_file=new std::fstream;
@)block_file->open(*argv++,binary_in);
  matrix_file->open(*argv++,binary_in);
  if (block_file->is_open() and matrix_file->is_open())
  { matrix_info::mode format; @< Determine the |format| of the matrix file @>
    if (format==matrix_info::transform)
      @< Reopen the |matrix_file| for reading and writing @>
    mi=std::auto_ptr<matrix_info>
       (new matrix_info(block_file,matrix_file,format));
    if (format==matrix_info::transform) // successfully read and transformed
      @< Mark matrix file as being in revised format @>
  }
  else
    { std::cerr << "failed to open file '"
		<< argv[block_file->is_open()? -1 : -2 ] << "'.\n";
      delete block_file; delete matrix_file; exit(1);
    }
}

@ The file format expected is determined by looking at the initial bytes of
the matrix file. We define a special magic value that indicates that the
format has been revised. While the update is in progress we write yet another
value to the file, so that file left after a crash in the midst of an update
is not mistaken for one in revised format.

@< Constant definitions @>=
const unsigned int magic_code=0x06ABdCF0;
const unsigned int work_in_progress=0x76543210;

@ The mechanism for requesting a transformation is voluntarily contrived,
since we want \.{KLread} to be a program that can be trusted not to modify any
files. Therefore to request the upgrade, the program itself must be renamed
(possibly via a symbolic link) to \.{KLwrite}.

@< Determine the |format|... @>=
{ unsigned int code=read_bytes<4>(*matrix_file);
  matrix_file->seekg(0,std::ios_base::beg); // reset to beginning of file
  format= code==magic_code ? matrix_info::revised :  matrix_info::old;
  if (format== matrix_info::old and program_name=="KLwrite")
    format= matrix_info::transform;
  if (code==work_in_progress)
    if (program_name=="KLwrite")
      std::cout << "Reattempting conversion of matrix file.\n";
    else
    @/{@; std::cout << "Broken matrix file, retry the conversion.\n";
     exit(1);
    }
  std::cout << "Matrix file format: "
    << ( format== matrix_info::old ? "old"
       : format== matrix_info::revised ? "new"
       : "updating to new" )
    << ".\n";
}

@
@< Reopen the |matrix_file| for reading and writing @>=
{ matrix_file->close();
  matrix_file->open(argv[-1],binary_in_out);
  if (not matrix_file->is_open())
  {  std::cerr << "failed to open file '"
		<< argv[-1] << "' for writing.\n";
      delete block_file; delete matrix_file; exit(1);
  }
  write_int(work_in_progress,*matrix_file);
  matrix_file->seekg(0,std::ios_base::beg); // reset to beginning of file
}

@
@< Mark matrix file as being in revised format @>=
{ matrix_file->seekg(0,std::ios_base::beg); // reset to beginning of file
  write_int(magic_code,*matrix_file);
  std::cout << "Conversion of matrix file successfully completed.\n";
}

@
@< Process use input, do |break| when done @>=
{
  if (mi.get()==NULL)
    std::cout << "index: ";
  else
    std::cout << "give block elements x,y, or polynomial index i as #i: ";

  while(isspace(std::cin.peek())) std::cin.get();

  KLIndex i;
  if (std::cin.peek()=='#' ? std::cin.get(), true : mi.get()==NULL )
    @< Read index~|i| from |std::cin|; if |"quit"| is found instead do
       |break|, and in case of errors |goto try_again| @>
  else
    @< Read in parameters |x,y|, and determine index~|i| of polynomial
       while printing the information found,
       or in anomalous cases |break| or |goto try_again| @>
  @< Locate the coefficients of polynomial~|i|,
     and print the polynomial @>
try_again:
  while(std::cin.peek()!=EOF and std::cin.get()!='\n') {}
  // skip to the end of the line

}

@
@< Read index~|i| from |std::cin|... @>=
{
  std::cin >> i;
  if (not std::cin.good())
  { std::string s; std::cin.clear(); std::cin>>s;
    if (s=="quit") break;
    std::cout << "index not correct.\n"; goto try_again;
  }
  if (i>=pol.n_polynomials())
  { std::cout << "index too large, limit is " << pol.n_polynomials()-1
	     << ".\n";
    goto try_again;
  }
}

@
@< Read in parameters |x,y|... @>=
{
  int c; BlockElt x=UndefBlock,y=UndefBlock,xx;
  while(ispunct(c=std::cin.peek()) or isspace(c)) std::cin.get();
  // skip spaces/punctuation
  std::cin >> x;
  if (x==UndefBlock) // non-numeric input
    @< Ask if user wishes to quit; if so |break|, else |goto try_again| @>

  while(ispunct(c=std::cin.peek()) or isspace(c)) std::cin.get();
  // skip spaces/punctuation
  std::cin >> y;
  if (y==UndefBlock)
    {@; std::cout << "failure reading y, try again.\n";
      goto try_again;
    }
  if (x>=mi->block_size())
  @/{@; std::cout << "first parameter too large, try again.\n";
      goto try_again;
    }
  if (y>=mi->block_size())
  @/{@; std::cout << "second parameter too large, try again.\n";
      goto try_again;
    }

  if (x>y)
    {@; std::cout << "Result null by triangularity.\n"; continue; }
@)
  i=mi->find_pol_nr(x,y,xx);
@)
  if (xx==UndefBlock) // this means |primitivise| hit a real non-parity case
    { std::cout
      << "Result is null because raising the first argument " @|
         "reaches a real non-parity case.\n";
      goto try_again;
    }
  else
    std::cout << "P_{" << x << ',' << y << "}=P_{" << xx << ',' << y @|
	      << "}=polynomial #" << i << ':' << std::endl;
}

@
@< Ask if user wishes to quit... @>=
{
  std::cin.clear();
  std::cout<< "Really quit? ";
  while(std::cin.peek()!=EOF and std::cin.get()!='\n') {}
  while(isspace(c)) std::cin.get();
  int ans=std::cin.peek();
  if (ans==EOF or ans=='y' or ans=='Y') break;
  goto try_again;
}

@ Here we catch the |runtime_error| values that could be thrown while getting
the coefficients of the polynomial. If that happens we print the message
thrown, but then continue normally.

@< Locate the coefficients of polynomial~|i|... @>=
try
{
  std::vector<ullong> coefficients(pol.coefficients(i));

  bool first=true;
  for (size_t i=coefficients.size(); i-->0;)
    if (coefficients[i]!=0)
    { if (first) first=false; else std::cout << " + ";
      if (coefficients[i]!=1 or i==0) std::cout << coefficients[i];
      std::cout << (i==0? "" : "q");
      if (i>1)
	if (i<10) std::cout << '^' << i;
	else std::cout << "^{" << i << '}';
    }
  if (coefficients.size()==0) std::cout << 0;
  else if (coefficients.size()>1)
  { unsigned int sum=0; // sum of coefficients, won't exceed $2^{29}$
    for (size_t i=0; i<coefficients.size(); ++i) sum+=coefficients[i];
    std::cout << "; value at q=1: " << sum;
  }
  std::cout << '.' << std::endl;
}
catch (std::runtime_error& e)
{ std::cerr << e.what(); }

@* Index.

% Local IspellDict: british