#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : data_({}), input_size_(0), output_size_(0), capacity_(capacity)
{ }

size_t ByteStream::write(const string &data) {
    // 已经写入完毕，则不能再写入
    if (input_ended()) return 0;
    // 可以写入的长度是data和缓冲区可用size二者的最小值
    size_t real_len = min(data.size(), remaining_capacity());
    for (size_t i = 0; i < real_len; ++i) {
        data_.push_back(data[i]);
    }
    input_size_ += real_len;
    return real_len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t real_len = min(len, buffer_size());
    string ret;
    for (size_t i = 0; i < real_len; ++i) {
        ret += data_[i];
    }
    return ret;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t real_len = min(len, buffer_size());
    for (size_t i = 0; i < real_len; ++i) {
        data_.pop_front();
    }
    // 需要在pop里设置对output_size的修改，而不是在read里，因为可能单独调用pop函数来对缓冲区进行pop操作而不一定要read
    output_size_ += len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    // 如果以及读到eof了，则不能再读入
    if (eof()) return {};
    // 可读取的len是输入参数和缓冲区当前容量二者的最小值
    size_t real_len = min(len, buffer_size());
    string ret = peek_output(real_len);
    pop_output(real_len);
    return ret;
}

void ByteStream::end_input() { input_end_ = true; }

bool ByteStream::input_ended() const { return input_end_; }

size_t ByteStream::buffer_size() const { return data_.size(); }

bool ByteStream::buffer_empty() const { return data_.empty(); }

// 当已经输入结束并且缓冲区也为空的时候证明输入结束并且读取结束，才可以设置eof
bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return input_size_; }

size_t ByteStream::bytes_read() const { return output_size_; }

size_t ByteStream::remaining_capacity() const {
    return capacity_ - buffer_size();
}
