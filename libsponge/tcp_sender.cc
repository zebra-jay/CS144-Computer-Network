#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <cstdio>

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
    , _stream(capacity) {
        _ackno = _isn;
        _RTO = _initial_retransmission_timeout;
    }

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    // here window is eff_window. only if _next_seqno==_abs_ackno, do we send one byte
    while ((_next_seqno < _abs_ackno+_eff_window_size()) && (not _fin_sent)) {
        TCPSegment seg;
        seg.header().seqno = wrap(_next_seqno, _isn);
        seg.header().syn = (_next_seqno==0);

        uint64_t payload_size = min({
            TCPConfig::MAX_PAYLOAD_SIZE,
            _abs_ackno+_eff_window_size()-_next_seqno - (seg.header().syn? 1: 0),
            _stream.buffer_size()
        });

        seg.payload() = _stream.read(payload_size);

        // fprintf(stderr, "FIN check: seq_space=%zu window_remaining=%zu\n", 
        //     seg.length_in_sequence_space(),
        //     _abs_ackno+_eff_window_size()-_next_seqno);


        if (_stream.eof()) {
            seg.header().fin = (seg.length_in_sequence_space()+1 <= _abs_ackno+_eff_window_size()-_next_seqno);
            _fin_sent = seg.header().fin;
        }
  
        if (seg.length_in_sequence_space() == 0) {
            break;
        }   

        _segments_out.push(seg);
        _outstanding_segments.push({{_next_seqno, static_cast<uint64_t>(_next_seqno + seg.length_in_sequence_space()-1)}, seg});
        
        _bytes_in_flight += seg.length_in_sequence_space();
        _next_seqno += seg.length_in_sequence_space();

    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {

    uint64_t _temp = unwrap(ackno, _isn, _abs_ackno);
    if (_temp > _next_seqno) return;
    if (_temp > _abs_ackno) {
        _ackno = ackno;
        _abs_ackno = _temp;
    }

    _window = window_size;
    _window_received = true;
    uint64_t last_acknowledged{};
    bool popped{};

    while ( not _outstanding_segments.empty() && (_outstanding_segments.front().first.first + _outstanding_segments.front().second.length_in_sequence_space()-1 < _abs_ackno) ) {
        popped = true;
        last_acknowledged = _outstanding_segments.front().first.first + _outstanding_segments.front().second.length_in_sequence_space()-1;
        _bytes_in_flight -= _outstanding_segments.front().second.length_in_sequence_space();
        _outstanding_segments.pop();
    }


    if (popped) {
    _abs_ackno = last_acknowledged+1;
    _ackno = wrap(_abs_ackno, _isn);

    _time = 0;
    _RTO = _initial_retransmission_timeout;
    _retransmissions = 0;
    }

    fill_window();

}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _time += ms_since_last_tick;
    if (_time >= _RTO) {
        _time = 0;
        if (_outstanding_segments.empty()){
            return;
        }

        _segments_out.push(_outstanding_segments.front().second);
        
        if (_window>0 || (not _window_received)) {
            _RTO*=2;
            _retransmissions++;
        }
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment empty;
    empty.header().seqno = wrap(_next_seqno, _isn);
    Buffer buf("");
    empty.payload() = buf;
    _segments_out.push(empty);
}