#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) { 
    
    /*Phase 0:
    init basic variables start, end, (sl, su, el, eu), 
    return in some base cases,
    declare variables to be used later
    for adding to map in case not directly attacheable: target, final_end
    for deletion of redundant indices: [delete_from, delete_until)
    for what is extra in buffer not in data: additional_string
    */
    size_t start = index, end = index+data.size();
    auto s_l = _auxiliary_buffer.lower_bound(start), s_u = _auxiliary_buffer.upper_bound(start);
    auto e_l = _auxiliary_buffer.lower_bound(end), e_u = _auxiliary_buffer.upper_bound(end);

    if (eof) _eof_received = true;
    if (eof) _last_index = end-1;
    if (_eof_received && (_reqd_index>_last_index || start==end)) {
        _output.end_input();
        return;
    }
    if (start==end) return; //empty string
    if (end<=_reqd_index) return; //already reassembled
    if (s_u==e_l && _is_end(s_u) && e_l!=_auxiliary_buffer.end() && !_auxiliary_buffer.empty()) {
        auto it = s_u;
        it--;
        if (it->first<=start && end<=e_l->first) return;
    } // substring of a string in auxiliary buffer
        
    size_t target{}, final_end{}, write_from{};
    bool target_is_start{};
    auto delete_from = _auxiliary_buffer.begin(), delete_until = _auxiliary_buffer.begin();
    string additional_string = "";

    /*Phase 1:
    for (s_l, s_u): assign target, delete_from
    for (e_l, e_u): assign final_end, delete_until & copy additional string
    */

    // for (s_l, s_u)
    if (_is_start(s_l)) {
        if (_is_start(s_u)) {
            target_is_start = true;
            target = start;
            delete_from = s_u;
            write_from = start;
        }
        else {
            target = s_l->first;
            delete_from = s_u;
            write_from = s_u->first;
        }
    }
    else if (_is_end(s_l)) {
        write_from = s_l->first;
        delete_from = s_l;
        s_l--;
        target = s_l->first;
    }
    else {
        target_is_start = true;
        target = start;
        delete_from = s_u;
        write_from = start;
    }

    // for (e_l, e_u)
    if (_is_start(e_l)) {
        if (_is_start(e_u)) {
            final_end = end;
            delete_until = e_l;
        }
        else {
            final_end = e_u->first;
            e_u++;
            delete_until = e_u;
            additional_string = e_l->second;
        }
    }
    else if (_is_end(e_l)) {
        final_end = e_l->first;
        e_l--;
        size_t map_string_start = e_l->first;
        string& map_string = e_l->second;
        for (size_t i = end-map_string_start; i<map_string.size(); i++) {
            additional_string += map_string[i];
        }
        e_l++;
        delete_until = ++e_l;
    }
    else {
        final_end = end;
        delete_until = e_l;
    }

    /*Phase 2:
    Find ranges to delete [delete_from, delete_until).
    Since we couldn't use iterators, we save the numbers in a vector then erase.
    it!=map.end() saves us from empty map
    */

    size_t bytes_removed{};
    vector<size_t> to_delete{};
    for (auto& it = delete_from; it!=delete_until && it!=_auxiliary_buffer.end(); it++) {
        bytes_removed += it->second.size();;
        to_delete.push_back(it->first);
    }
    for (auto key: to_delete) _auxiliary_buffer.erase(key);
    _auxiliary_size -= bytes_removed;

    /*Phase 3:
    add to target
    */

    // if attacheable write directly to ByteStream
    if (start<=_reqd_index && _reqd_index<end && !_is_full()) {
        for (size_t i = _reqd_index-start; i<data.size() && !_is_full(); i++) {
            _output.write(_char_as_string(data[i]));
            _reqd_index++;
            
            if (_eof_received && _reqd_index>_last_index) _output.end_input();
        }
        for (size_t i = 0; i<additional_string.size() && !_is_full(); i++) {
            _output.write(_char_as_string(additional_string[i]));
            _reqd_index++;

            if (_eof_received && _reqd_index>_last_index) _output.end_input();
        }
    }
    // else add to map at target
    else if(!_is_full()){
        if(target_is_start) _auxiliary_buffer[target] = "";
        for (size_t i = write_from-start; i<data.size() && !_is_full(); i++) {
            _auxiliary_buffer[target]+=data[i];
            _auxiliary_size++;
        }
        for (size_t i = 0; i<additional_string.size() && !_is_full(); i++) {
            _auxiliary_buffer[target]+=additional_string[i];
            _auxiliary_size++;
        } 
        _auxiliary_buffer[final_end] = "";
    }

    
    return; 
}

size_t StreamReassembler::unassembled_bytes() const { return _auxiliary_size; }

bool StreamReassembler::empty() const { return _auxiliary_size==0; }