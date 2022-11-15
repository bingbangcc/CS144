#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

 /*
  * 主要是弄清楚三个序号的关系，比如要传送目标数据流abcde，则其在tcp协议中传送的实际数据流是(syn)abcde(fin)
  * 流序号：指的是数据在整个数据流里的索引，只包含数据部分没有syn和fin，是uint64，无循环
  * 绝对序号：指的是包含syn和fin的实际数据流中的某字节的索引，是uint64，无循环
  * 序号：指的是在tcp报文头里标识的实际数据流中某字节的索引，特点是uint32，有循环，其标志的是相对isn的位置关系
  * 整个流程是：tcp报文头里存储的是序号，而_reassembler进行组装所使用的是流序号，绝对序号是连接二者的桥梁
  * 序号和绝对序号只是用来标识流数据各个字节所对应的索引位置，其并未是在流数据的头尾增加了某些字段，数据自始至终都只有那些
  * 例子：
  * 序号:    0 1 2 0 1 2
  * 绝对序号：0 1 2 3 4 5
  * 流序号：    0 1 2 3
  * 流数据：    a b c d
  * */
void TCPReceiver::segment_received(const TCPSegment &seg) {
    TCPHeader tcp_header = seg.header();
    // 第一次收到SYN报文，将syn位置为true，并且设置isn（初始序列号）
    if (!syn_) {
        if (!tcp_header.syn) return;
        syn_ = true;
        // 第一次收到syn报文的时候，这里的seqno指向的就是(syn)字节
        isn_ = tcp_header.seqno;
    }

    // bytes_written是已经写入的个数
    uint64_t checkout = _reassembler.stream_out().bytes_written();
    // 确定该段里报文的绝对索引,绝对序列号里包含了流最前面的syn的一个序号
    uint64_t obsolute_index = unwrap(tcp_header.seqno, isn_, checkout);
    // 确定流索引，如果当前是带有(syn)的则流序号为0，否则是绝对序号-1
    uint64_t stream_index = obsolute_index - 1 + tcp_header.syn;
    //
//    uint32_t start_pos = tcp_header.syn;
    // (syn)只占用一个序号，并不是真的存储在数据段，因此data就是该段的数据部分
    std::string data = seg.payload().copy();
    // 这里传进去fin只是说明已经从发送方接收到所有数据，但有的数据可能还没组装，因此并不是说fin传进来就要将确认号绝对序号+1
    // 第一个syn报文，其data是空的，直接调用下面的函数也可以得到正确的执行结果
    _reassembler.push_substring(data, stream_index, tcp_header.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!syn_) return std::nullopt;
    // 下一个待读取的字节序号，是流序号
    uint64_t stream_index = _reassembler.next_index();
    // 绝对序号等于流序号+1
    uint64_t obsolute_index = stream_index + 1;
    // 如果已经将所有数据读取完毕，则此时fin也占用一个序号，则绝对序号应该+1
    if (_reassembler.stream_out().input_ended()) obsolute_index += 1;
    WrappingInt32 ack_sequence = wrap(obsolute_index, isn_);
    return ack_sequence;
}

size_t TCPReceiver::window_size() const {
    return _reassembler.stream_out().remaining_capacity();
}
