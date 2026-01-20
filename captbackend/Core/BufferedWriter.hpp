#pragma once
#include <ostream>
#include <streambuf>
#include <span>

class BufferedWriter : public std::streambuf {
private:
    std::ostream& dest;

    int_type overflow(int_type c = traits_type::eof()) noexcept override;
    int sync() noexcept override;
public:
    // dest stream MUST be noexcept
    explicit BufferedWriter(std::ostream& dest, std::span<char_type> buffer) noexcept;
};
