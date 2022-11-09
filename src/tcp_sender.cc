#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , time_rto_(retx_timeout) {}

uint64_t TCPSender::bytes_in_flight() const {
    return bytes_in_flight_;
}

void TCPSender::fill_window() {
    // 这里都是从字节流中读取数据构造新的报文段，重发的报文不是在这，重发的可以直接访问unfinished_segments_来获取
    // 如果该报文是syn报文，则不带数据段，并设置相应头，发送完直接返回
    if (!set_syn_) {
        // 构建tcp报文
        TCPSegment seg;
        seg.header().seqno = next_seqno();
        seg.header().syn = true;
        _next_seqno++;
        set_syn_ = true;
        _segments_out.push(seg);
        unfinished_segments_.push(seg);
        bytes_in_flight_++;
        return;
    }
    // 接下来处理数据部分的报文段
    // 当前窗口的大小，也就是可以传输的数据的长度
    size_t cur_window_size_ = window_size_ ? window_size_ : 1;
    // 需要判断是否需要发送新的报文段，只有在window_size足够大以及fin并未设置的时候才能发送报文段
    // window_size是receiver可以接收的序号范围，而bytes_in_flight_是已经发送但还没确定的序号范围
    // 因此此时可以发送的序号空间是window_size-bytes_in_flight_
    while (!set_fin_ && cur_window_size_ > bytes_in_flight_) {
        size_t stream_size = min(cur_window_size_ - bytes_in_flight_, _stream.buffer_size());
        stream_size = min(stream_size, TCPConfig::MAX_PAYLOAD_SIZE);
        TCPSegment seg;
        seg.header().seqno = next_seqno();
        string data = _stream.read(stream_size);
        seg.payload() = Buffer(std::move(data));
        if (_stream.eof()) {
            seg.header().fin = true;
            set_fin_ = true;
        }
        // 如果该报文的序号是空的，这意味着该报文既不带数据也不是fin或者syn，则这样的报文没用不需要要发送
        // 这种情况可能出现在sender的流缓冲区没有字节能读取了，并且还不能设置fin
        if (seg.length_in_sequence_space() == 0) {
            break;
        }
        _next_seqno += seg.length_in_sequence_space();
        bytes_in_flight_ += seg.length_in_sequence_space();
        _segments_out.push(seg);
        unfinished_segments_.push(seg);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
// 这里window_size代表的是序号的个数，而lab2里实现的window_size返回的是缓冲区的可用空间即数据的个数
// 这里sender和receiver的window_size定义有一点差异，导致send这里接收到的window_size会比实际reveriver想告诉的
// 数据部分的window_size小一点，但并不影响实际功能的实现，因为sender发送的数据可能少一点
// 不会导致receiver缓冲区异常等情况，而且省去了考虑syn和fin的麻烦
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    window_size_ = window_size;
    // 确认号的绝对序号
    uint64_t ackno_abs = unwrap(ackno, _isn, _next_seqno);
    // 对还未发送的字节进行确认，因此直接返回
    if (ackno_abs > _next_seqno) {
        return;
    }
    // 将所有被该ack号完全累计确认的段从unfinished_segments_中pop
    // 这里队列里已经是按照绝对序号排序好的，因此只要循环判断队头元素即可
    while (!unfinished_segments_.empty()) {
        TCPSegment temp_seg = unfinished_segments_.front();
        uint64_t seq_abs = unwrap(temp_seg.header().seqno, _isn, _next_seqno);
        // 该段所包含的最后一个序号
        seq_abs += temp_seg.length_in_sequence_space()-1;
        // 如果该段已经被完全确认，则将该段从queue里pop，并且减少当前已经发送但未被确认的序列号数量
        if (ackno_abs > seq_abs) {
            unfinished_segments_.pop();
            bytes_in_flight_ -= temp_seg.length_in_sequence_space();
            // 收到ack之后需要将定时器重置
            time_count_ = 0;
            time_rto_ = _initial_retransmission_timeout;
            consecutive_retransmission_times_ = 0;
        }
        // 如果队头段没有被完全确认则没有什么操作执行，继续等待对该段的确认
        else {
            break;
        }
    }
    // 从ack包里得到了当前接收方的新window_size，这个新的window可能比之前更大
    // 使得发送方可以发送新的数据，因此需要调用fill_window
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // 先计算当前time_count的值
    time_count_ += ms_since_last_tick;
    // 如果时间超过rto则需要将队头段进行重传，并更新rto以及time_count
    if (!unfinished_segments_.empty() && time_count_ >= time_rto_) {
        TCPSegment temp_seq = unfinished_segments_.front();
        _segments_out.push(temp_seq);
        if (window_size_ != 0)
            time_rto_ *= 2;
        time_count_ = 0;
        consecutive_retransmission_times_++;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const {
    return consecutive_retransmission_times_;
}

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    _segments_out.push(seg);
}
