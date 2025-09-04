#include "UsbStreambuf.hpp"
#include "Core/Log.hpp"
#include "UsbError.hpp"
#include <cassert>
#include <cstddef>
#include <cstring>
#include <libusb.h>

using int_type = UsbStreambuf::int_type;

UsbStreambuf::UsbStreambuf(UsbPrinter& printer, std::size_t buffSize, unsigned timeoutMs)
    : printer(printer), rbuff(buffSize), wbuff(buffSize), timeoutMs(timeoutMs) {
    char_type* wstart = this->wbuff.data();
    char_type* wend = wstart + this->wbuff.size();
    this->setp(wstart, wend);
}

int_type UsbStreambuf::overflow(int_type c) {
    if (c == traits_type::eof()) {
        return traits_type::eof();
    }
    this->sync();
    *this->pptr() = traits_type::to_char_type(c);
    this->pbump(1);
    return c;
}

int_type UsbStreambuf::underflow() {
    if (this->gptr() < this->egptr()) {
        return traits_type::to_int_type(*this->gptr());
    }
    assert(this->printer.handle.get() != nullptr);
    std::size_t maxCount = this->rbuff.size();
    char_type* start = this->rbuff.data();
    int transferred;
    int err = libusb_bulk_transfer(
        this->printer.handle.get(), this->printer.readEp,
        reinterpret_cast<unsigned char*>(start), maxCount, &transferred,
        this->timeoutMs
    );
    if (err != LIBUSB_SUCCESS) {
        Log::Debug() << "UsbStreambuf::underflow(): libusb_bulk_transfer failed: " << libusb_error_name(err);
        throw UsbError(err);
    }
    assert(transferred >= 0);
    Log::Debug() << "Received " << transferred << " from device";
    this->setg(start, start, start + transferred);
    return traits_type::to_int_type(*this->gptr());
}

int UsbStreambuf::sync() {
    if (this->pptr() == this->pbase()) {
        return 0;
    }
    assert(this->printer.handle.get() != nullptr);
    std::ptrdiff_t count = this->pptr() - this->pbase();
    int result = count;

    while (count > 0) {
        int transferred;
        int err = libusb_bulk_transfer(
            this->printer.handle.get(), this->printer.writeEp,
            reinterpret_cast<unsigned char*>(this->pbase()), count, &transferred,
            this->timeoutMs
        );
        if (err != LIBUSB_SUCCESS) {
            Log::Debug() << "UsbStreambuf::sync(): libusb_bulk_transfer failed: " << libusb_error_name(err);
            throw UsbError(err);
        }
        assert(transferred >= 0 && transferred <= count);
        Log::Debug() << "Sent " << transferred << " bytes to device";
        count -= transferred;
    }

    this->pbump(-result);
    assert(this->pptr() == this->pbase());
    return result;
}
