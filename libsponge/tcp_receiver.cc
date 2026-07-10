#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.


using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    TCPHeader header = seg.header();
    Buffer payload = seg.payload();

    if (header.syn) _isn = header.seqno;
    if (header.fin) _eof_received = true;

    uint64_t absolute_index = unwrap(WrappingInt32(header.seqno), _isn.value(), _eof_received);


    // if (not _isn.has_value()) // No SYN received yet
    // {

    // }
    // else if (_isn.has_value() && (not (_reassembler.stream_out()).input_ended())) // Receiving
    // {

        // _reassembler.push_substring(payload, absolute_index-1, _eof_received);

    // }
    // else if (_reassembler.stream_out().input_ended()) {} // Ended 
    
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_isn.has_value()) {
        uint64_t required_index = _reassembler.stream_out().bytes_written();
        return wrap(required_index, _isn.value());
    }
    return nullopt;
}

size_t TCPReceiver::window_size() const {
    return _capacity - _reassembler.stream_out().buffer_size();
}