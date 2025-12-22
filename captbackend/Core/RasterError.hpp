#pragma once
#include <stdexcept>

class RasterError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
