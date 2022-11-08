#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    // 截取绝对序列号的低32位作为序列号的偏移量，直接调用类WrappingInt32的加法即可
    // 截取后32位是因为，高32位每个1都相当于在32位环上循环了某些整环
    // 因此最后落在32位环上的偏移量是一样的
    auto temp = static_cast<uint32_t>(n);
    return isn + temp;
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    // 这里n的绝对序列号肯定比isn的绝对序列号要大，isn的绝对序列号是0
    WrappingInt32 check = wrap(checkpoint, isn);
    // 这里的关键是注意距离checkpoint最近的元素其实有三个环的待选元素
    // 即对于目标值tar来说，其距离checkpoint最近的可能是tar-2^32，tar，tar+2^32三个元素是可能的结果
    // 此外如何判断选择哪个环上的值可以根据距离diff与环大小的一半即2^31来进行比较选择
    // 例如:
    // ....n.|..t.n.|....n.
    // .....n|.t...n|.....n
    // .n....|.n...t|.n....
    // ...n..|...n.t|...n..
    uint32_t diff = n - check;
    uint64_t ans = checkpoint + diff;
    if (diff > (1u << 31) && ans >= (1ul << 32)) {
        ans -= (1ul << 32);
    }
    return ans;
}
