#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().buffer_size(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return time_since_last_segment_received_; }

State TCPConnection::get_tcp_state() {
    string sender = TCPState::state_summary(_sender), receiver = TCPState::state_summary(_receiver);
    if (receiver == TCPReceiverStateSummary::LISTEN &&
        sender == TCPSenderStateSummary::CLOSED) return State::LISTEN;
    if (receiver == TCPReceiverStateSummary::SYN_RECV &&
        sender == TCPSenderStateSummary::SYN_SENT) return State::SYN_RCVD;
    if (receiver == TCPReceiverStateSummary::LISTEN  &&
        sender == TCPSenderStateSummary::SYN_SENT) return State::SYN_SENT;
    if (receiver == TCPReceiverStateSummary::SYN_RECV &&
        sender == TCPSenderStateSummary::SYN_ACKED) return State::ESTABLISHED;
    if (receiver == TCPReceiverStateSummary::FIN_RECV &&
        sender == TCPSenderStateSummary::SYN_ACKED && !_linger_after_streams_finish) return State::CLOSE_WAIT;
    if (receiver == TCPReceiverStateSummary::FIN_RECV &&
        sender == TCPSenderStateSummary::FIN_SENT && !_linger_after_streams_finish) return State::LAST_ACK;
    if (receiver == TCPReceiverStateSummary::FIN_RECV &&
        sender == TCPSenderStateSummary::FIN_SENT) return State::CLOSING;
    if (receiver == TCPReceiverStateSummary::SYN_RECV &&
        sender == TCPSenderStateSummary::FIN_SENT) return State::FIN_WAIT_1;
    if (receiver == TCPReceiverStateSummary::SYN_RECV &&
        sender == TCPSenderStateSummary::FIN_ACKED) return State::FIN_WAIT_2;
    if (receiver == TCPReceiverStateSummary::FIN_RECV &&
        sender == TCPSenderStateSummary::FIN_ACKED) return State::TIME_WAIT;
    if (receiver == TCPReceiverStateSummary::ERROR &&
        sender == TCPSenderStateSummary::ERROR) return State::RESET;
    if (receiver == TCPReceiverStateSummary::FIN_RECV &&
        sender == TCPSenderStateSummary::FIN_ACKED) return State::CLOSED;
    return {};
}

void TCPConnection::send_segments() {
    // 如果当前没有要发送的报文段，那就发送一个空包进行keep-alive
    if (_sender.segments_out().empty()) {
        _sender.send_empty_segment();
    }
    while (!_sender.segments_out().empty()) {
        auto seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = _receiver.window_size();
        }
        _segments_out.push(seg);
    }
}

// 分不同的阶段进行处理
void TCPConnection::segment_received(const TCPSegment &seg) {
    if (!active()) return;
    time_since_last_segment_received_ = 0;
    // 如果收到rst报文，则直接中断该连接
    if (seg.header().rst) {
        set_rst_state(false);
        return;
    }

    // 对每个阶段进行分别处理，这里底层实现的_receiver.segment_received和_sender.ack_received不够鲁棒
    State tcp_state = get_tcp_state();
    TCPHeader header = seg.header();
    // 在LISTEN阶段收到对方的SYN报文，与对面建立连接，则本方为服务端
    if (tcp_state == State::LISTEN) {
        if (header.syn) {
            connect();
            _linger_after_streams_finish = false;
        }
    }
    // 本机作为客户端主动发送SYN报文
    else if (tcp_state == State::SYN_SENT) {
        // 服务器应发送SYN+ACK报文
        if (header.ack && header.syn) {
            _receiver.segment_received(seg);
            _sender.ack_received(header.ackno, header.win);
            send_segments();
        }
    }
    else if (tcp_state == State::SYN_RCVD) {
        _receiver.segment_received(seg);
        if (header.ack) {
            _sender.ack_received(header.ackno, header.win);
        }
        send_segments();
    }
    // 服务器端已经收到了FIN报文
    else if (tcp_state == State::CLOSE_WAIT) {
        // 之前的ACK报文丢失
        if (header.fin) {
            _sender.send_empty_segment();
            send_segments();
            return;
        }
        _receiver.segment_received(seg);
        if (header.ack) {
            _sender.ack_received(header.ackno, header.win);
        }
        send_segments();
    }
    else if (tcp_state == State::LAST_ACK) {
        _receiver.segment_received(seg);
        if (header.ack) {
            _sender.ack_received(header.ackno, header.win);
        }
        is_alive_ = false;
        return;
    }
    else if (tcp_state == State::FIN_WAIT_1) {

    }
    else if (tcp_state == State::FIN_WAIT_2) {

    }
    else if (tcp_state == State::TIME_WAIT) {

    }
    // 如果是正常连接阶段，则直接将报文段相应部分分发给sender和receiver
    else {
        _receiver.segment_received(seg);
        if (seg.header().ack) {
            _sender.ack_received(seg.header().seqno, seg.header().win);
        }
        send_segments();
    }
}

bool TCPConnection::active() const { return is_alive_; }

size_t TCPConnection::write(const string &data) {
    size_t write_size = _sender.stream_in().write(data);
    _sender.fill_window();
    send_segments();
    return write_size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if (!active()) return;
     time_since_last_segment_received_ += ms_since_last_tick;
     // 更新时间戳，如果有需要则重复相应的报文段
     _sender.tick(ms_since_last_tick);
     // 达到最大重传次数，则关闭该tcp连接
     if (_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS) {
         set_rst_state(true);
         return;
     }
     send_segments();
     // 如果time_wait已经过了最大等待时间，则set_rst
     State tcp_stat = get_tcp_state();
     if (tcp_stat == State::TIME_WAIT) {
         if (time_since_last_segment_received_ > 10*_cfg.rt_timeout) {
             set_rst_state(false);
             return;
         }
     }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    send_segments();
}

void TCPConnection::connect() {
    _sender.fill_window();
    is_alive_ = true;
    send_segments();
}

void TCPConnection::set_rst_state(bool send_rst_seg) {
    if (send_rst_seg) {
        TCPSegment seg;
        seg.header().rst = true;
        _segments_out.push(seg);
    }
    is_alive_ = false;
    _linger_after_streams_finish = false;
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            set_rst_state(true);
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
