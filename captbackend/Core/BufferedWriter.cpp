#include "BufferedWriter.hpp"
#include <cassert>
#include <ios>

using int_type = BufferedWriter::int_type;

BufferedWriter::BufferedWriter(std::ostream& dest, std::span<char_type> buffer) noexcept : dest(dest) {
    assert(dest.exceptions() == std::ios_base::goodbit);
    this->setp(buffer.data(), buffer.data() + buffer.size() - 1);
}

int_type BufferedWriter::overflow(int_type c) noexcept {
    if (!traits_type::eq_int_type(c, traits_type::eof())) {
        *this->pptr() = traits_type::to_char_type(c);
        this->pbump(1);
    }
    return this->sync() == 0 ? traits_type::not_eof(c) : traits_type::eof();
}

int BufferedWriter::sync() noexcept {
    if (this->dest.good()) {
        std::ptrdiff_t count = this->pptr() - this->pbase();
        this->dest.write(this->pbase(), count);
        this->setp(this->pbase(), this->epptr());
        return 0;
    }
    return 1;
}
