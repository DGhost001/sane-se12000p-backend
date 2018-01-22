#ifndef SANEDEVICEHANDLE_H
#define SANEDEVICEHANDLE_H

#include "parallelport.hpp"
#include "a4s2600.hpp"
#include "scannercontrol.hpp"


class SaneDeviceHandle
{
public:
    SaneDeviceHandle(const std::string &devName);
    ~SaneDeviceHandle();

    ParallelPortSpp& getParaport();
    A4s2600& getAsic();
    ScannerControl& getScanner();

private:
    ParallelPortSpp paraport_;
    A4s2600 *asic_;
    ScannerControl *scanner_;
};

#endif // SANEDEVICEHANDLE_H
