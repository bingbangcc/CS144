#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <list>
//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.


/*
 * 将目标字符流切分为一些切片进行传输
 * StreamReassemble接收这些切片，并在自己的内存空间中维持，这些切片可以不按顺序发送接收
 * 一旦next_index指向的字节到达，意味着StreamReassemble可以将从next_index_开始的一段连续的字符流输入到ByteStream中
 * 输入到ByteStream之后，接收方就可以采用ByteStream提供的read接口进行读取，这读取到的都是有顺序的，保证了可靠
 * 1. StreamReassemble也不是无限大的，其也有容量限制，这里其最大容量设置为和ByteStream一样的_capacity
 * 2. StreamReassemble里有一个ByteStream对象，用来将从next_index_开始的一段字符write到ByteStream里
 * 3. 写入ByteStream的时候，需要根据此段的长度和ByteStream可写入的长度来修改next_index_
 * 4. 在接收切片的时候，可能涉及到不同切片之间的重叠问题以及合并问题，本质就是一个区间的合并问题
 * 5. 这里直接使用map，其key是index，value是str，str隐含提供了区间的长度，map自动按key来排序了
 * 6.
 *
 * */
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.

    ByteStream _output;  //!< The reassembled in-order byte stream
    // 这里capacity同时指定了StreamReassembler本身以及其要输入的ByteStream的最大容积
    // 这二者的最大容积都是_capacity
    size_t _capacity;    //!< The maximum number of bytes
    size_t next_index_ = 0;  // 下一个待插入字节的index
    size_t unassenbled_size_ = 0;
    size_t eof_index_ = -1;
    bool eof_flag_ = false;

    // 用来记录unassenbled的strs
    // (begin, end) -> end-begin+1 = str.len
    struct node{
        size_t begin_;
        size_t end_;
        std::string str;
        bool operator < (const node& other) {
            return this->begin_ < other.begin_;
        }
    };
    std::list<node> unassenbled_strs_ = {};

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;
    void judge_eof(const std::string &data, const size_t index, const bool eof);
    uint64_t next_index() const { return next_index_; }
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
