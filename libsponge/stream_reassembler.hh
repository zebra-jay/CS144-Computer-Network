#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <string>

#include <deque>
#include <map>
#include <vector>

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.

    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes
    
    size_t _required_index{}; // the 0-based index that comes immediately after the in-order bytestream in _output
                            // for example: if _output = "abc" with index of c in incoming bytestream as 3, reqd index = 4
    size_t last_index_of_stream{}; // the last index of the incoming bytestream. once you see this index, you know that the stream has ended. this is only set when eof is true in push_substring
    size_t _auxiliary_capacity() { return _capacity - _output.buffer_size();} // byte capacity the reassembler has to work with
    size_t _auxiliary_size{};                                                  // bytes currently with the reassembler
    size_t _remaining_auxiliary_size() { return _auxiliary_capacity() - _auxiliary_size; }  // bytes we can add more to the reassembler before it's full
    size_t _size() { return _auxiliary_size + _output.buffer_size(); }
    std::map<size_t, std::string> _auxiliary_storage{};                   // containg <(start or end), string as deque>
    bool _is_full() { return _size()==_capacity; }

    std::string _char_as_string(char ch) {
      std::string s{};
      s+=ch;
      return s;
    }

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receives a substring and writes any newly contiguous bytes into the stream.
    //!
    //! If accepting all the data would overflow the `capacity` of this
    //! `StreamReassembler`, then only the part of the data that fits will be
    //! accepted. If the substring is only partially accepted, then the `eof`
    //! will be disregarded.
    //!
    //! \param data the string being added
    //! \param index the index of the first byte in `data`
    //! \param eof whether or not this segment ends with the end of the stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been submitted twice, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH