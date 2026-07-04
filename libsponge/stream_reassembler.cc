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

    // start: index of first character in whole stream
    // end  : index of last  character in whole stream
    size_t start = index, end = index+data.size()-1; 

    if (eof) last_index_of_stream = end; // if eof is true, we know the last index of the stream. we can use this to check if we have reached the end of the stream.

    if (last_index_of_stream<=_required_index) _output.end_input(); // if we have already reassembled upto the end of the stream, we can end the input.

    // if 1. you've already reassembled upto end i.e. end <= reqd.index = last_reassembled_index+1
    // or 2. you don't have any space left either auxiliary or reassembled bytestream
    // or 3. data is empty, then we don't need to do anything. return.
    // or 4. data is substring of a string in auxiliary. i.e. upper_bound(start)==upper_bound(end)!=map.end() and up_bnd(start) is an empty
    if (end < this->_required_index || this->_is_full() || data.size()==0) return;
    
    //is data directly attacheable, so start <= reqd.index <= end. if so, we can attach to bytestream and then check if we can attach more from auxiliary storage.
    bool can_attach_directly = (start <= this->_required_index) && (this->_required_index <= end);
    if (can_attach_directly) // attach to bytestream 
    {
        string additional_string{}; // since we can't push_back to data, we'll push_back to this after data in all cases
        // size_t string_iter = this->_required_index - start; 
        auto it = _auxiliary_storage.upper_bound(end);
        size_t until = it->first;// for begin it is <until for end it is <=end
        
        if (it->second.empty()) { // it is an end. we can attach some parts.
            size_t copy_string_end = it->first; 
            it--;
            size_t copy_string_start = it->first;
            const string &copy_string = it->second;


            for (size_t i = end - copy_string_start + 1; copy_string_start + i <= copy_string_end; i++) additional_string.push_back(copy_string[i]); 
            it++;
        }

        // now that we have taken all deletable strings, we delete them
        vector<size_t> to_remove;
        for (auto iter = _auxiliary_storage.begin(); iter->first < until; iter++) to_remove.push_back(iter->first);
        if (it->second.empty()) to_remove.push_back(until);
        for (const auto &_index: to_remove) { // why shadows a parameter. shouldn't scope resolution come into play?
            _auxiliary_size -= _auxiliary_storage[_index].size();
            _auxiliary_storage.erase(_index);
        }

        //now that we have freed up space from auxiliary, we push to _object reassembled bystream while we can
        //bytestream handles its own tracking
        for (size_t i = 0; i < data.size() && !this->_is_full(); i++)  {
            _output.write(_char_as_string(data[i]));
            if (i==data.size()-1 && eof) { // basically start+i==end here. so we can end.
                _output.end_input();
                return;
            }

        } 
        for (size_t i = 0; i < additional_string.size() && !this->_is_full(); i++) {
            _output.write(_char_as_string(additional_string[i]));
            if (end+1+i==last_index_of_stream && eof) { // basically start+i==end here. so we can end.
                _output.end_input();
                return;
            }
        }

    }
    else 
    {
        //our data is is ahead of reqd. character. now we will have to merge with other substrings if possible.
        //same method as above 

        auto end_in_map   = _auxiliary_storage.upper_bound(end);
        auto start_in_map = _auxiliary_storage.upper_bound(start);

        if (start_in_map==end_in_map && start_in_map!=_auxiliary_storage.end() && start_in_map->second.empty()) return; // data is substring of a string in auxiliary. we don't need to do anything.

        string additional_string{};

        // -> is defined here as upper bounds to for eg: in [3, 5] 3 upper bounds to index 1, i.e. 5
        // if data.end -> map_string.end, we first copy from the string stored in previous key which is a start, from the index after data.end
        if (end_in_map!=_auxiliary_storage.end() && end_in_map->second.empty() && end_in_map!=start_in_map) {
            const size_t map_end = end_in_map->first;
            end_in_map--;
            const string &map_string = end_in_map->second;
            const size_t map_start = end_in_map->first;
            
            //copy from after index end until map_end
            for (size_t i = end-map_start+1; map_start+i <= map_end; i++) additional_string.push_back(map_string[i]);

            end_in_map++;
        }

        // now we have data and additional string. we just push back both into map[new_start] depending on upper_bound(start)
        auto iter = _auxiliary_storage.begin();
        size_t target{};
        bool target_is_start = false;
        size_t start_index{};
        if (start_in_map==_auxiliary_storage.end() || !start_in_map->second.empty()) { // insert start into map. new_string
            target_is_start = true;
            target = start;
            iter = start_in_map;
            start_index = start;
        }
        else { //insert into what is already there
            start_index = start_in_map->first + 1;
            target = (start_in_map--)->first;
            iter = start_in_map++;
        }



        // now that we have taken all deletable strings, we delete them
        vector<size_t> to_remove;
        for (; iter!=_auxiliary_storage.end() && iter!=end_in_map; iter++) to_remove.push_back(iter->first);
        if (end_in_map!=_auxiliary_storage.end() && end_in_map->second.empty()) to_remove.push_back(end_in_map->first);
        for (const auto &_index: to_remove) {
            _auxiliary_size -= _auxiliary_storage[_index].size(); //accumulate this beforehand.
            _auxiliary_storage.erase(_index);
        }

        if (target_is_start) {
            _auxiliary_storage[target] = "";
            for (size_t i = 0; i < data.size() && !this->_is_full(); i++) {
                _auxiliary_storage[target]+=data[i];
                _auxiliary_size++;
            }
            for (size_t i = 0; i < additional_string.size() && !this->_is_full(); i++) {
                _auxiliary_storage[target]+=additional_string[i];
                _auxiliary_size++;
            }
        }
        else{
            for (size_t i = start_index-start; i < data.size() && !this->_is_full(); i++){
                _auxiliary_storage[target]+=data[i];
                _auxiliary_size++;
            }
            for (size_t i = 0; i < additional_string.size() && !this->_is_full(); i++) {
                _auxiliary_storage[target]+=additional_string[i];
                _auxiliary_size++;
            }

        }
    }   

}

size_t StreamReassembler::unassembled_bytes() const { return _auxiliary_size; }

bool StreamReassembler::empty() const { return _auxiliary_size==0; }