#include  "filekl.h"
#include  "blocks.h"
#include  "basic_io.h"
#include  "kl.h"

namespace atlas {
  namespace filekl {
    
    void write_block_file(const Block& block, std::ostream& out)
    {
      unsigned char rank=block.rank(); // certainly fits in a byte
    
      basic_io::put_int(block.size(),out);  // block size in 4 bytes
      out.put(rank);                        // rank in 1 byte
    
      { // output length data
        unsigned char max_length=block.length(block.size()-1);
        out.put(max_length);
    
        BlockElt z=0;
        // |basic_io::put_int(z,out)|; obvious: no elements |z| with |length(z)<0|
        for (size_t l=0; l<max_length; ++l)
        {
          while(block.length(z)<=l)
    	++z;
          basic_io::put_int(z,out); // record: z elements of length<=l
        }
        assert(z<block.size());
    
        // |basic_io::put_int(block.size(),out);|
        // also obvious: there are block.size() elements of |length<=max_length|
      }
    
      // write descent sets
      for (BlockElt y=0; y<block.size(); ++y)
      {
        RankFlags d;
        for (size_t s = 0; s < rank; ++s)
          d.set(s,block.isWeakDescent(s,y));
        basic_io::put_int(d.to_ulong(),out); // write d as 32-bits value
      }
    
      // write table of primitivatisation successors
      for (BlockElt x=0; x<block.size(); ++x)
      {
    #if VERBOSE
        std::cerr << x << '\r';
    #endif
        for (size_t s = 0; s < rank; ++s)
        {
          DescentStatus::Value v = block.descentValue(s,x);
          if (DescentStatus::isDescent(v)
    	  or v==DescentStatus::ImaginaryTypeII)
    	basic_io::put_int(noGoodAscent,out);
          else if (v == DescentStatus::RealNonparity)
    	basic_io::put_int(blocks::UndefBlock,out);
          else if (v == DescentStatus::ComplexAscent)
    	basic_io::put_int(block.cross(s,x),out);
          else if (v == DescentStatus::ImaginaryTypeI)
    	basic_io::put_int(block.cayley(s,x).first,out);
          else assert(false);
        }
      }
    }
    
    std::streamoff
    write_KL_row(const kl::KLContext& klc, BlockElt y, std::ostream& out)
    {
      BitMap prims=klc.primMap(y);
      const kl::KLRow& klr=klc.klRow(y);
    
      assert(klr.size()+1==prims.capacity()); // check the number of KL polynomials
    
      // write row number for consistency check on reading
      basic_io::put_int(y,out);
    
      std::streamoff start_row=out.tellp();
    
      // write number of primitive elements for convenience
      basic_io::put_int(prims.capacity(),out);
    
      // now write the bitmap as a sequence of unsigned int values
      for (size_t i=0; i<prims.capacity(); i+=32)
        basic_io::put_int(prims.range(i,32),out);
    
      // finally, write the indices of the KL polynomials themselves
      for (size_t i=0; i<klr.size(); ++i)
      {
        assert((klr[i]!=0)==prims.isMember(i));
        if (klr[i]!=0) // only write nonzero indices
          basic_io::put_int(klr[i],out);
      }
    
      basic_io::put_int(1,out); // write unrecorded final polynomial 1
    
      // and signal if there was unsufficient space to write the row
      if (not out.good()) throw error::OutputError();
    
      return start_row;
    }
    
    void write_matrix_file(const kl::KLContext& klc, std::ostream& out)
    {
      std::vector<unsigned int> delta(klc.size());
      std::streamoff offset=0;
      for (BlockElt y=0; y<klc.size(); ++y)
      {
        std::streamoff new_offset=write_KL_row(klc,y,out);
        delta[y]=static_cast<unsigned int>((new_offset-offset)/4);
        offset=new_offset;
      }
    
      // now write the values allowing rapid location of the matrix rows
      for (BlockElt y=0; y<klc.size(); ++y)
        basic_io::put_int(delta[y],out);
    
      // and finally sign file as being in new format by overwriting 4 bytes
      out.seekp(0,std::ios_base::beg);
      basic_io::put_int(magic_code,out);
    }
    
    void write_KL_store(const kl::KLStore& store, std::ostream& out)
    {
      const size_t coef_size=4; // dictated (for now) by |basic_io::put_int|
    
      basic_io::put_int(store.size(),out); // write number of KL poynomials
    
      // write sequence of 5-byte indices, computed on the fly
      size_t offset=0; // position of first coefficient written
      for (size_t i=0; i<store.size(); ++i)
      {
        kl::KLPolRef p=store[i]; // get reference to polynomial
    
        // output 5-byte value of offset
        basic_io::put_int(offset&0xFFFFFFFF,out);
        out.put(char(offset>>16>>16)); // >>32 would fail on 32 bits machines
    
        if (not p.isZero()) // superfluous since polynomials::MinusOne+1==0
          // add number of coefficient bytes to be written
          offset += (p.degree()+1)*coef_size;
      }
      // write final 5-byte value (total size of coefficiant list)
      basic_io::put_int(offset&0xFFFFFFFF,out); out.put(char(offset>>16>>16));
    
      // now write out coefficients
      for (size_t i=0; i<store.size(); ++i)
      {
        kl::KLPolRef p=store[i]; // get reference to polynomial
        if (not p.isZero())
          for (size_t j=0; j<=p.degree(); ++j)
    	basic_io::put_int(p[j],out);
      }
    }

  }
}

