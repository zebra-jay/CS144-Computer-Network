#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.


using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    TCPHeader header = seg.header();
    Buffer payload = seg.payload();

    if (header.syn) _isn = header.seqno;
    // if (header.fin) _eof_received = true;

    
    if (_isn.has_value() && (not (_reassembler.stream_out()).input_ended())) // Receiving
    {
    WrappingInt32 start_index(header.seqno + (header.syn? 1 : 0));
    uint64_t absolute_index = unwrap(start_index, _isn.value(), _bytes_assembled());
    _reassembler.push_substring(payload.copy(), absolute_index-1, header.fin);

    }
    
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_isn.has_value()) {
        uint64_t string_required_index = _reassembler.stream_out().bytes_written();
        uint64_t absolute_required_index = string_required_index+1;
        uint64_t offset = fin_offset();
        return wrap(absolute_required_index+offset, _isn.value()); // not an optional? okay?
    }
    return nullopt;
}

size_t TCPReceiver::window_size() const {
    return _capacity - _reassembler.stream_out().buffer_size();
}