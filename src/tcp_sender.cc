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
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const {
    return bytes_in_flight_;
}

void TCPSender::fill_window() {

}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    DUMMY_CODE(ackno, window_size);
    window_size_ = window_size ? window_size : 1;
    // 确认号的绝对序号
    uint64_t ackno_abs = unwrap(ackno, _isn, _next_seqno);
    // 将所有被该ack号完全累计确认的段从unfinished_segments_中pop
    // 这里队列里已经是按照绝对序号排序好的，因此只要循环判断队头元素即可
    while (!unfinished_segments_.empty()) {
        TCPSegment temp_seg = unfinished_segments_.front();
        uint64_t seq_abs = unwrap(temp_seg.header().seqno, _isn, _next_seqno);
        // 该段索包含的最后一个序号
        seq_abs += temp_seg.length_in_sequence_space()-1;
        // 如果该段已经被完全确认，则将该段从queue里pop，并且减少当前已经发送但未被确认的序列号数量
        if (ackno_abs > seq_abs) {
            unfinished_segments_.pop();
            bytes_in_flight_ -= temp_seg.length_in_sequence_space();
        }

    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    DUMMY_CODE(ms_since_last_tick);

}

unsigned int TCPSender::consecutive_retransmissions() const {
    return retransmission_times_;
}

void TCPSender::send_empty_segment() {}
