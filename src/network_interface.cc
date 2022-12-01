#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    // 如果本机保存了目标ip的MAC地址，则直接造帧并发送
    auto iter = arp_table_.find(next_hop_ip);
    if (iter != arp_table_.end()) {
        EthernetFrame frame;
        frame.header().dst = iter->second.first;
        frame.header().src = _ethernet_address;
        frame.header().type = EthernetHeader::TYPE_IPv4;
        frame.payload() = dgram.serialize();
        _frames_out.push(frame);
    }
    // 如果没有目标ip的MAC地址映射
    // 则需要先进行arp广播获取目标MAC再进行构造帧并发送
    else {
        // 先将报文和ip存入cache中
//        cache_data_.push_back({next_hop, dgram});
        cache_data_[next_hop_ip].push(dgram);
        // 查看目标ip是否已经被arp了，如果没有则需要进行arp广播
        if (waits_frames_.find(next_hop_ip) == waits_frames_.end()) {
            // ARP报文被放在MAC帧的数据部分，因为MAC帧的头部仅包含目的MAC和源MAC
            ARPMessage arp_message;
            arp_message.opcode = ARPMessage::OPCODE_REQUEST;
            arp_message.sender_ethernet_address = _ethernet_address;
            arp_message.sender_ip_address = _ip_address.ipv4_numeric();
            arp_message.target_ip_address = next_hop_ip;

            EthernetFrame broadcast_frame;
            // 这里是arp广播帧，因此目的MAC地址是全1
            broadcast_frame.header().dst = ETHERNET_BROADCAST;
            broadcast_frame.header().src = _ethernet_address;
            broadcast_frame.header().type = EthernetHeader::TYPE_ARP;
            broadcast_frame.payload() = BufferList(arp_message.serialize());
            _frames_out.push(broadcast_frame);
            // 记录进行arp广播的当前时间
            waits_frames_[next_hop_ip] = time_;
        }
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    // 如果传进来的MAC帧的目标MAC地址不是本主机并且不是广播帧，则直接忽略
    EthernetAddress eth_addr = frame.header().dst;
    if (eth_addr != _ethernet_address && eth_addr != ETHERNET_BROADCAST) {
        return std::nullopt;
    }

    if (frame.header().type == EthernetHeader::TYPE_ARP) {

        ARPMessage message;
        ParseResult result = message.parse(frame.payload());
        if (result != ParseResult::NoError) return std::nullopt;
        // 这里不管是request还是reply都需要更新arp表项
        arp_table_[message.sender_ip_address] = {message.sender_ethernet_address, time_};
        // 如果arp帧里的目标主机ip地址不是本主机则不需要进行回复
        // 如果是request则需要进行回复
        if (message.opcode == ARPMessage::OPCODE_REQUEST && message.target_ip_address == _ip_address.ipv4_numeric()) {
            ARPMessage arp_message;
            arp_message.opcode = ARPMessage::OPCODE_REPLY;
            arp_message.sender_ethernet_address = _ethernet_address;
            arp_message.sender_ip_address = _ip_address.ipv4_numeric();
            arp_message.target_ip_address = message.sender_ip_address;
            arp_message.target_ethernet_address = message.sender_ethernet_address;
            EthernetFrame reply_frame;
            // 这里是arp单播帧
            reply_frame.header().dst = message.sender_ethernet_address;
            reply_frame.header().src = _ethernet_address;
            reply_frame.header().type = EthernetHeader::TYPE_ARP;
            reply_frame.payload() = BufferList(arp_message.serialize());
            _frames_out.push(reply_frame);
            // arp回复帧不需要进行计时，因此不需要再arp_table里进行更新
        }
        // 目标frame的ip->MAC映射可能导致之前等待的某些帧可以进行发送
        if (waits_frames_.find(message.sender_ip_address) != waits_frames_.end()) {
            waits_frames_.erase(message.sender_ip_address);
            std::queue<InternetDatagram> temp_que = cache_data_[message.sender_ip_address];
            while (!temp_que.empty()) {
                // 这是想要发送的帧
                InternetDatagram dgram = temp_que.front();
                temp_que.pop();
                EthernetFrame send_frame;
                send_frame.header().dst = message.sender_ethernet_address;
                send_frame.header().src = _ethernet_address;
                send_frame.header().type = EthernetHeader::TYPE_IPv4;
                send_frame.payload() = dgram.serialize();
                _frames_out.push(send_frame);
            }
            cache_data_.erase(message.sender_ip_address);
            waits_frames_.erase(message.sender_ip_address);
        }
        return std::nullopt;
    }
    // 收到的是ip报文，则提取这个报文并将其返回
    InternetDatagram ip_gram;
    ParseResult result = ip_gram.parse(frame.payload());
    if (result != ParseResult::NoError) return std::nullopt;
    return ip_gram;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    time_ += ms_since_last_tick;
    // 将waits_frames里等待超过5秒的进行重发
    for (auto& [ip, time] : waits_frames_) {
        // 如果超过5秒则进行重传
        if (time_ - time > 5000) {
            ARPMessage arp_message;
            arp_message.opcode = ARPMessage::OPCODE_REQUEST;
            arp_message.sender_ethernet_address = _ethernet_address;
            arp_message.sender_ip_address = _ip_address.ipv4_numeric();
            arp_message.target_ip_address = ip;

            EthernetFrame broadcast_frame;
            // 这里是arp广播帧，因此目的MAC地址是全1
            broadcast_frame.header().dst = ETHERNET_BROADCAST;
            broadcast_frame.header().src = _ethernet_address;
            broadcast_frame.header().type = EthernetHeader::TYPE_ARP;
            broadcast_frame.payload() = BufferList(arp_message.serialize());
            _frames_out.push(broadcast_frame);
            // 记录进行arp广播的当前时间
            waits_frames_[ip] = time_;
        }
    }

    // 将arp_table里超过30秒的进行删除
    for (auto iter = arp_table_.begin(); iter != arp_table_.end(); ){
        if (time_ - iter->second.second > 30000) {
            iter = arp_table_.erase(iter);
        }
        else iter++;
    }
}
