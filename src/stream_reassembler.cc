#include "stream_reassembler.hh"



// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // StreamReassembler没有空间了
    if (index >= next_index_ + _capacity) return;
    // 当前插入的都是next_index之前的没用数据，这里需要判断一下是否需要设置eof
    // 因为有可能是前面的都把数据传输完了，最后传一个空字符串并设置eof为true的情况
    if (index+data.size() <= next_index_) {
        judge_eof(data, index, eof);
        return;
    }

    size_t real_index = index, real_len = 0;
    string real_data = data;
    if (index < next_index_) {
        real_index = next_index_;
        real_data = data.substr(next_index_-index);
    }

    // 最多可以插入的str的实际长度
    real_len = min(real_data.size(), _capacity + next_index_ - real_index);
    if (real_len == 0) return;
    // 插入待处理的切片，其key是该str的(begin_index, end_index)，value是对应的str
    unassenbled_strs_.push_back({real_index, real_index+real_len-1, data.substr(real_index-index, real_len)});
    // 接下来对map里的数据进行处理以使得所有区间都没有重叠
    unassenbled_strs_.sort();
    std::list<node> temp;
    for (auto& c : unassenbled_strs_) {
        if (temp.empty() || c.begin_ > temp.back().end_) {
            temp.emplace_back(c);
        }
        else if (c.end_ <= temp.back().end_)
            continue;
        else {
            temp.back().str += c.str.substr(temp.back().end_-c.begin_+1);
            temp.back().end_ = c.end_;
        }
    }
    unassenbled_strs_ = std::move(temp);
    // 这里需要添加unassenbled_strs_不为空
    // 不断读入数据到ByteStream里，并且修改unassenbled_strs_的首项
    while (!unassenbled_strs_.empty() && next_index_ == unassenbled_strs_.front().begin_) {
        size_t write_size = _output.write(unassenbled_strs_.front().str);
        if (write_size == 0)
            break;
        else if (write_size == unassenbled_strs_.front().str.size()) {
            next_index_ += unassenbled_strs_.front().str.size();
            unassenbled_strs_.pop_front();
        }
        else {
            next_index_ += write_size;
            unassenbled_strs_.front().begin_ += write_size;
            unassenbled_strs_.front().str = unassenbled_strs_.front().str.substr(write_size);
        }
    }

    // 记录还有多少元素缓存在未组装区
    unassenbled_size_ = 0;
    for (auto& c : unassenbled_strs_) {
        unassenbled_size_ += c.str.size();
    }

    judge_eof(data, index, eof);
}

void StreamReassembler::judge_eof(const std::string &data, const size_t index, const bool eof) {
    if (eof) {
        eof_index_ = index + data.size();
    }
    if (next_index_ == eof_index_) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const {
    return unassenbled_size_;
}

bool StreamReassembler::empty() const {
    return unassenbled_size_ == 0;
}

