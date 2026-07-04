#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

using namespace std;

ByteStream::ByteStream(const size_t capacity) { 
    this->_buffer_capacity = capacity;
}

size_t ByteStream::write(const string &data) {
    
    size_t num_bytes_written = 0;
    for (const auto &it: data) {
        if (this->_buffer_size < this->_buffer_capacity) {
            num_bytes_written++;
            this->_buffer_size++;
            this->_buffer.push_back(it);
        }
        else break;
    }
    this->_bytes_written += num_bytes_written;
    return num_bytes_written;

}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string peek;
    size_t cnt = 0;
    for (const auto &it: this->_buffer) {
        if (cnt>=len) break;

        peek.push_back(it);
        cnt++;
    }
    return peek;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t num_bytes_popped = 0;
    for (size_t i = 0; i < len && !this->buffer_empty(); i++) {
        num_bytes_popped++;
        this->_buffer.pop_front();
    }
    this->_buffer_size -= num_bytes_popped;
    this->_bytes_read += num_bytes_popped;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string output; //try this with iterators
    size_t num_bytes_popped = 0;
    for (size_t i = 0; i < len && !this->buffer_empty(); i++) {
        num_bytes_popped++;
        output.push_back(this->_buffer.front()); //this returns a reference to the first element, not a copy
        this->_buffer.pop_front();
    }
    this->_buffer_size -= num_bytes_popped;
    this->_bytes_read += num_bytes_popped;
    
    return output;
}

void ByteStream::end_input() { this->_input_ended = true; }

bool ByteStream::input_ended() const { return this->_input_ended; }

size_t ByteStream::buffer_size() const { return this->_buffer_size; }

bool ByteStream::buffer_empty() const { return this->_buffer_size==0 ? true : false; }

bool ByteStream::eof() const { return this->buffer_empty() && this->input_ended(); }

size_t ByteStream::bytes_written() const { return this->_bytes_written; }

size_t ByteStream::bytes_read() const { return this->_bytes_read; }

size_t ByteStream::remaining_capacity() const { return this->_buffer_capacity - this->_buffer_size; }