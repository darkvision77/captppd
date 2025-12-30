#pragma once
#include <ostream>
#include <streambuf>
#include <span>

class BufferedWriter : public std::streambuf {
private:
    std::ostream& dest;

    int_type overflow(int_type c = traits_type::eof()) override;
    int sync() override;
public:
    explicit BufferedWriter(std::ostream& dest, std::span<char_type> buffer) noexcept;
};
