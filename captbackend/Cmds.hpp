#ifndef _CAPTBACKEND_CMDS_HPP_
#define _CAPTBACKEND_CMDS_HPP_

#include "Core/CaptPrinter.hpp"
#include <stop_token>

int CmdPrint(std::stop_token stopToken, StateReporter& reporter, CaptPrinter& printer, int rasterFd);
int CmdClean(std::stop_token stopToken, CaptPrinter& printer);

#endif
