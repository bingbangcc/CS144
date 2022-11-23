#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

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
//    if (receiver == TCPReceiverStateSummary::FIN_RECV &&
//        sender == TCPSenderStateSummary::SYN_ACKED) return State::CLOSE_WAIT;
//    if (receiver == TCPReceiverStateSummary::FIN_RECV &&
//        sender == TCPSenderStateSummary::FIN_SENT) return State::LAST_ACK;
    if (receiver == TCPReceiverStateSummary::FIN_RECV &&
        sender == TCPSenderStateSummary::FIN_SENT) return State::CLOSING;
    if (receiver == TCPReceiverStateSummary::SYN_RECV &&
        sender == TCPSenderStateSummary::FIN_SENT) return State::FIN_WAIT_1;
    if (receiver == TCPReceiverStateSummary::SYN_RECV &&
        sender == TCPSenderStateSummary::FIN_ACKED) return State::FIN_WAIT_2;
    if (receiver == TCPReceiverStateSummary::FIN_RECV &&
        sender == TCPSenderStateSummary::FIN_ACKED) return State::TIME_WAIT;
    if (receiver == TCPReceiverStateSummary::ERROR &&
        sender == TCPSenderStateSummary::ERROR && !_linger_after_streams_finish && !is_alive_) return State::RESET;
    if (receiver == TCPReceiverStateSummary::FIN_RECV &&
        sender == TCPSenderStateSummary::FIN_ACKED && !_linger_after_streams_finish && !is_alive_) return State::CLOSED;
    return {};
}

void TCPConnection::send_segments() {

    while (!_sender.segments_out().empty()) {
        TCPSegment seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = _receiver.window_size();
        }
        _segments_out.push(seg);
    }

    // 如果已经接收结束
    if (_receiver.stream_out().input_ended()) {
        // 如果发送没有结束，那么是服务器端，不需要计时器
        if (!_sender.stream_in().eof()) {
            _linger_after_streams_finish = false;
        }
        // 如果发送已经结束。
        // 如果是服务端，则可以直接结束
        // 如果是客户端，则超过计时间隔可以结束
        else if (_sender.bytes_in_flight() == 0) {
            if (!_linger_after_streams_finish || time_since_last_segment_received_ >= 10*_cfg.rt_timeout) {
                is_alive_ = false;
            }
        }
    }

}

// 关键在于，每当收到一个报文都要进行ack，如果当前sender已经没有数据要发送，那么就需要发送一个空包
// 如果接收到一个空包，那么说明对方的sender目前没有数据进行发送，如果seg.length_in_sequence_space() > 0
// 那么需要对该报文进行ack，如果seg.length_in_sequence_space() == 0那么就说明这个报文仅仅是对你已发送得以一个报文的
// ack，是不需要对其发送一个空包进行ack的。
// 空包只是进行ack的手段，收到空包的话不需要进行回复
// 分不同的阶段进行处理

void TCPConnection::segment_received(const TCPSegment &seg) {
    if (!is_alive_) {
        return;
    }

    time_since_last_segment_received_ = 0;

    if (seg.header().rst) {
        set_rst_state(false);
        return;
    }

    // 这些状态分析的时候，一边考虑对方可以接收到你之前发送的报文，可以正确的进行ACK所采取的动作
    // 一边考虑如果之前的报文段还没到达对面可能产生的动作，这里不用考虑丢失的问题是因为底层实现了重传机制
    // 可以保证对方最后肯定会收到相应的报文

    State tcp_state = get_tcp_state();
    TCPHeader header = seg.header();

    // 只在第一次握手的时候ACK为false
    // 一旦建立连接，则此后ACK一直都是true，因此不用在后续节点进行甄别ACK是否为true

    // closed/listen
    if (tcp_state == State::LISTEN) {
        // 只有在收到SYN报文的时候才进行回应，否则直接pass
        if (header.syn) {
            _receiver.segment_received(seg);
            // 这里还没有ACK，不需要sender进行操作
            connect();
        }
    }
    else if (tcp_state == State::SYN_SENT) {
        if (header.syn && header.ack) {
            _receiver.segment_received(seg);
            _sender.ack_received(header.ackno, header.win);
            // 因为收到了SYN+ACK报文，序号数肯定不为0，因此必须进行回复ACK，又因为可能本方sender的缓冲区为0，这样在sender
            // 的底层实现里不会发送报文段，因此为了保证ack则必须发送一个空报文段
            _sender.send_empty_segment();
            send_segments();
        }
        // 这个是本方发送的SYN丢失，然后接收到了对方第一次握手的SYN报文，此时没有ACK，因此不需要sender的解析操作
        else if (header.syn) {
            _receiver.segment_received(seg);
            _sender.send_empty_segment();
            send_segments();
        }
    }
    else if (tcp_state == State::SYN_RCVD) {
        // 本方为服务端，并且已经收到了对方的SYN
        // 如果对方发送的不是SYN报文，则说明已经建立了连接，直接分析报文信息即可
        if (!header.syn) {
            _receiver.segment_received(seg);
            _sender.ack_received(header.ackno, header.win);
            if (seg.length_in_sequence_space() > 0) {
                _sender.send_empty_segment();
            }
            send_segments();
        }
        else if (header.syn) {
            _receiver.segment_received(seg);
            // 因为有SYN则必须进行ACK
            _sender.send_empty_segment();
            send_segments();
        }
//        _receiver.segment_received(seg);
//        _sender.ack_received(header.ackno, header.win);
//        send_segments();
    }
    else if (tcp_state == State::ESTABLISHED) {
        _receiver.segment_received(seg);
        // 这里肯定是有ACK的
        _sender.ack_received(header.ackno, header.win);
        // 如果有序号的话则需要进行ACK，如果对面发送的是keep-alive的ACK空包的话，则不需要进行ACK
        if (seg.length_in_sequence_space() > 0) {
            _sender.send_empty_segment();
        }
        send_segments();
    }
    else if (tcp_state == State::CLOSE_WAIT) {
        // 此时已经收到对面的FIN报文
        // 进行ACK的报文还没到达，又收到对面的FIN报文，则进行ACK即可
        if (header.fin) {
//            _receiver.segment_received(seg);
            // 这里FIN是有序号的，因此需要进行ACK
            _sender.ack_received(header.ackno, header.win);
            _sender.send_empty_segment();
            send_segments();
        }
        // 这说明对面已经收到我们对他FIN报文的ACK，因此对方不会再发送数据，每次都仅对我们的报文发送空包进行ACK
        else {
            // 这里服务端的receiver已经接收到了所有数据，不需要再对空包进行分析了
//            _receiver.segment_received(seg);
            _sender.ack_received(header.ackno, header.win);
            // 这里对面发送的仅仅是对我们进行ACK的空包，因此没有序号，不需要我们进行ACK，因此不用发送确认空包
            send_segments();
        }
    }
    else if (tcp_state == State::LAST_ACK) {
        // 此时我们作为服务端ACK了对面的FIN报文，并且已经发送了FIN报文
        // 我们仅需要在收到对面对我们最后一个数据的ACK之后关闭连接即可
        // 这里发送的也是空包ACK，因此不需要进行receiver的分析
//        _receiver.segment_received(seg);

        _sender.ack_received(header.ackno, header.win);
        // 如果服务端sender的没有处于 "已经发送但还没被确认" 的数据则说明已经完成了最后的ACK，可以关闭连接
//        if (_sender.bytes_in_flight() == 0) {
//            is_alive_ = false;
//            return;
//        }
        // 可能有需要重发的报文段，因此在没关闭连接之前的所有阶段都需要调用send_segments
        send_segments();
    }
    else if (tcp_state == State::FIN_WAIT_1) {
        // 此时我们作为客户端发送了FIN报文
        // 说明对面是正常进行ACK的，但这个ACK报文可能是空包也可能是带有数据的包
        if (!header.fin) {
            _receiver.segment_received(seg);
            _sender.ack_received(header.ackno, header.win);
            // 没有FIN的话，只有在有序号的时候才进行ACK
            if (seg.length_in_sequence_space() > 0) {
                _sender.send_empty_segment();
            }
            send_segments();
        }
        else if (header.fin) {
            // 对方也进行了FIN
            _receiver.segment_received(seg);
            // 如果对方发送了FIN报文，则FIN报文里也可能有ACK，需要进行sender分析
            // 除了第一次握手的SYN报文之外，其余所有报文的ACK都是true的
            _sender.ack_received(header.ackno, header.win);
            _sender.send_empty_segment();
            send_segments();
        }
    }
    else if (tcp_state == State::FIN_WAIT_2) {
        // 此时我们已经收到了对方对于我们FIN报文的ACK
        // 此时我们需要等待对方发送FIN报文，我们进行ACK，然后进入time_wait状态
        // 如果对方发送了FIN报文
        if (header.fin) {
            _receiver.segment_received(seg);
            // FIN报文里没有ACK，不能进行sender的分析
            if (header.ack) {
                _sender.ack_received(header.ackno, header.win);
            }
            // FIN报文消耗序号，必须进行ACK
            _sender.send_empty_segment();
            send_segments();
        }
        else {
            // 对方发送的是数据报文段
            _receiver.segment_received(seg);
            _sender.ack_received(header.ackno, header.win);
            // 如果有消耗序号的话，则必须进行ACK
            if (seg.length_in_sequence_space() > 0) {
                _sender.send_empty_segment();
            }
            send_segments();
        }
    }
    else if (tcp_state == State::TIME_WAIT) {
        // 此时客户端正在进行等待
        // 这说明服务端重发了FIN报文
        if (header.fin) {
            _receiver.segment_received(seg);
            // 有ACK，需要传给sender分析
            _sender.ack_received(header.ackno, header.win);
            _sender.send_empty_segment();
//            send_segments();
        }
        send_segments();
    }
    else if (tcp_state == State::CLOSING) {
        // 这个是因为客户端发送了FIN，但是服务端也发送了FIN
        // 此时已经收到对面的FIN报文，则对方不会再发送数据，因此不用receiver
//        _receiver.segment_received(seg);
        // 这说明之前我们对FIN报文的ACK对面没有收到，则ACK一下
        if (header.fin) {
            _receiver.segment_received(seg);
            _sender.ack_received(header.ackno, header.win);
            _sender.send_empty_segment();
            send_segments();
        }
        else {
            // 对方发送的是数据报文段
            _receiver.segment_received(seg);
            _sender.ack_received(header.ackno, header.win);
            // 如果有消耗序号的话，则必须进行ACK
            if (seg.length_in_sequence_space() > 0) {
                _sender.send_empty_segment();
            }
            send_segments();
        }
    }
    else {
        _receiver.segment_received(seg);
        _sender.ack_received(header.ackno, header.win);
        if (seg.length_in_sequence_space() > 0) {
            _sender.send_empty_segment();
        }
        send_segments();
    }

//
//    if (tcp_state == State::LISTEN) {
//        // passive open
//        if (seg.header().syn) {
//            // todo Don't have ack no, so sender don't need ack
//            _receiver.segment_received(seg);
//            connect();
////            log_print("closed -> syn-rcvd");
//        }
//        return;
//    }
//
//    // syn-sent
//    if (tcp_state == State::SYN_SENT) {
//        if (seg.header().syn && seg.header().ack) {
//            // active open
//            _sender.ack_received(seg.header().ackno, seg.header().win);
//            _receiver.segment_received(seg);
//            // send ack
//            _sender.send_empty_segment();
//            send_segments();
//            // become established
////            log_print("syn-sent -> established");
//        } else if (seg.header().syn && !seg.header().ack) {
//            // simultaneous open
//            _receiver.segment_received(seg);
//            // already send syn, need a ack
//            _sender.send_empty_segment();
//            send_segments();
//            // become syn_rcvd
////            log_print("syn-sent -> syn_rcvd");
//        }
//        return;
//    }
//
//    // syn-rcvd
//    if (tcp_state == State::SYN_RCVD) {
//        // receive ack
//        // todo need ack
//        _receiver.segment_received(seg);
//        _sender.ack_received(seg.header().ackno, seg.header().win);
////        log_print("syn-rcvd -> established");
//        return;
//    }
//
//    // established, aka stream ongoing
//    if (tcp_state == State::ESTABLISHED) {
//        _sender.ack_received(seg.header().ackno, seg.header().win);
//        _receiver.segment_received(seg);
//        if (seg.length_in_sequence_space() > 0) {
//            _sender.send_empty_segment();
//        }
//        _sender.fill_window();
//        send_segments();
////        if (seg.header().fin) {
////            log_print("established -> close wait");
////        }
//        return;
//    }
//
//    // close wait
//    if (tcp_state == State::CLOSE_WAIT) {
//        _sender.ack_received(seg.header().ackno, seg.header().win);
//        _receiver.segment_received(seg);
//        // try to send remain data
//        _sender.fill_window();
//        send_segments();
////        log_print("close wait -> (send remain data) or (last ack)");
//        return;
//    }
//
//    // FIN_WAIT_1
//    if (tcp_state == State::FIN_WAIT_1) {
//        if (seg.header().fin && seg.header().ack) {
//            _sender.ack_received(seg.header().ackno, seg.header().win);
//            _receiver.segment_received(seg);
//            _sender.send_empty_segment();
//            send_segments();
////            log_print("fin_wait_1 -> time_wait");
//        } else if (seg.header().fin && !seg.header().ack) {
//            _sender.ack_received(seg.header().ackno, seg.header().win);
//            _receiver.segment_received(seg);
//            _sender.send_empty_segment();
//            send_segments();
////            log_print("fin_wait_1 -> closing");
//        } else if (!seg.header().fin && seg.header().ack) {
//            _sender.ack_received(seg.header().ackno, seg.header().win);
//            _receiver.segment_received(seg);
//            send_segments();
////            log_print("fin_wait_1 -> fin_wait_2");
//        }
//
//        return;
//    }
//
//    // CLOSING
//    if (tcp_state == State::CLOSING) {
//        _sender.ack_received(seg.header().ackno, seg.header().win);
//        _receiver.segment_received(seg);
//        send_segments();
////        log_print("closing -> time_wait");
//        return;
//    }
//
//    // FIN_WAIT_2
//    if (tcp_state == State::FIN_WAIT_2) {
//        _sender.ack_received(seg.header().ackno, seg.header().win);
//        _receiver.segment_received(seg);
//        _sender.send_empty_segment();
//        send_segments();
////        log_print("fin_wait_2 -> time_wait");
//        return;
//    }
//
//    // TIME_WAIT
//    if (tcp_state == State::TIME_WAIT) {
//        if (seg.header().fin) {
//            _sender.ack_received(seg.header().ackno, seg.header().win);
//            _receiver.segment_received(seg);
//            _sender.send_empty_segment();
//            send_segments();
////            log_print("time_wait -> time_wait (Still reply FIN)");
//        }
//
//        return;
//    }
//    // 有些状态没有预判到，这里统一处理下。
//    _sender.ack_received(seg.header().ackno, seg.header().win);
//    _receiver.segment_received(seg);
//    _sender.fill_window();
//    send_segments();
}

//void TCPConnection::segment_received(const TCPSegment &seg) {
//    if (!is_alive_) {
//        return;
//    }
//
//    time_since_last_segment_received_ = 0;
//
//    if (seg.header().rst) {
//        set_rst_state(false);
//        return;
//    }
//
//    // closed
//    if (_sender.next_seqno_absolute() == 0) {
//        // passive open
//        if (seg.header().syn) {
//            // todo Don't have ack no, so sender don't need ack
//            _receiver.segment_received(seg);
//            connect();
////            log_print("closed -> syn-rcvd");
//        }
//        return;
//    }
//
//    // syn-sent
//    if (_sender.next_seqno_absolute() == _sender.bytes_in_flight() && !_receiver.ackno().has_value()) {
//        if (seg.header().syn && seg.header().ack) {
//            // active open
//            _sender.ack_received(seg.header().ackno, seg.header().win);
//            _receiver.segment_received(seg);
//            // send ack
//            _sender.send_empty_segment();
//            send_segments();
//            // become established
////            log_print("syn-sent -> established");
//        } else if (seg.header().syn && !seg.header().ack) {
//            // simultaneous open
//            _receiver.segment_received(seg);
//            // already send syn, need a ack
//            _sender.send_empty_segment();
//            send_segments();
//            // become syn_rcvd
////            log_print("syn-sent -> syn_rcvd");
//        }
//        return;
//    }
//
//    // syn-rcvd
//    if (_receiver.ackno().has_value() && !_receiver.stream_out().input_ended() &&
//        _sender.next_seqno_absolute() == _sender.bytes_in_flight()) {
//        // receive ack
//        // todo need ack
//        _receiver.segment_received(seg);
//        _sender.ack_received(seg.header().ackno, seg.header().win);
////        log_print("syn-rcvd -> established");
//        return;
//    }
//
//    // established, aka stream ongoing
//    if (_sender.next_seqno_absolute() > _sender.bytes_in_flight() && !_sender.stream_in().eof()) {
//        _sender.ack_received(seg.header().ackno, seg.header().win);
//        _receiver.segment_received(seg);
//        if (seg.length_in_sequence_space() > 0) {
//            _sender.send_empty_segment();
//        }
//        _sender.fill_window();
//        send_segments();
//        if (seg.header().fin) {
////            log_print("established -> close wait");
//        }
//        return;
//    }
//
//    // close wait
//    if (!_sender.stream_in().eof() && _receiver.stream_out().input_ended()) {
//        _sender.ack_received(seg.header().ackno, seg.header().win);
//        _receiver.segment_received(seg);
//        // try to send remain data
//        _sender.fill_window();
//        send_segments();
////        log_print("close wait -> (send remain data) or (last ack)");
//        return;
//    }
//
//    // FIN_WAIT_1
//    if (_sender.stream_in().eof() && _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 &&
//        _sender.bytes_in_flight() > 0 && !_receiver.stream_out().input_ended()) {
//        if (seg.header().fin && seg.header().ack) {
//            _sender.ack_received(seg.header().ackno, seg.header().win);
//            _receiver.segment_received(seg);
//            _sender.send_empty_segment();
//            send_segments();
////            log_print("fin_wait_1 -> time_wait");
//        } else if (seg.header().fin && !seg.header().ack) {
//            _sender.ack_received(seg.header().ackno, seg.header().win);
//            _receiver.segment_received(seg);
//            _sender.send_empty_segment();
//            send_segments();
////            log_print("fin_wait_1 -> closing");
//        } else if (!seg.header().fin && seg.header().ack) {
//            _sender.ack_received(seg.header().ackno, seg.header().win);
//            _receiver.segment_received(seg);
//            send_segments();
////            log_print("fin_wait_1 -> fin_wait_2");
//        }
//
//        return;
//    }
//
//    // CLOSING
//    if (_sender.stream_in().eof() && _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 &&
//        _sender.bytes_in_flight() > 0 && _receiver.stream_out().input_ended()) {
//        _sender.ack_received(seg.header().ackno, seg.header().win);
//        _receiver.segment_received(seg);
//        send_segments();
////        log_print("closing -> time_wait");
//        return;
//    }
//
//    // FIN_WAIT_2
//    if (_sender.stream_in().eof() && _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 &&
//        _sender.bytes_in_flight() == 0 && !_receiver.stream_out().input_ended()) {
//        _sender.ack_received(seg.header().ackno, seg.header().win);
//        _receiver.segment_received(seg);
//        _sender.send_empty_segment();
//        send_segments();
////        log_print("fin_wait_2 -> time_wait");
//        return;
//    }
//
//    // TIME_WAIT
//    if (_sender.stream_in().eof() && _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 &&
//        _sender.bytes_in_flight() == 0 && _receiver.stream_out().input_ended()) {
//        if (seg.header().fin) {
//            _sender.ack_received(seg.header().ackno, seg.header().win);
//            _receiver.segment_received(seg);
//            _sender.send_empty_segment();
//            send_segments();
////            log_print("time_wait -> time_wait (Still reply FIN)");
//        }
//
//        return;
//    }
//    // 有些状态没有预判到，这里统一处理下。
//    _sender.ack_received(seg.header().ackno, seg.header().win);
//    _receiver.segment_received(seg);
//    _sender.fill_window();
//    send_segments();
//}


bool TCPConnection::active() const { return is_alive_; }

size_t TCPConnection::write(const string &data) {
//    if (data.size() == 0) return 0;
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

}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    send_segments();
}

void TCPConnection::connect() {
    is_alive_ = true;
    _sender.fill_window();
    send_segments();
}

void TCPConnection::set_rst_state(bool send_rst_seg) {
    if (send_rst_seg) {
        TCPSegment seg;
        seg.header().rst = true;
        _segments_out.push(seg);
    }
//    std::cout << send_rst_seg << std::endl;
    is_alive_ = false;
//    _linger_after_streams_finish = false;
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
