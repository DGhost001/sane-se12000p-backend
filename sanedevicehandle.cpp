#include "sanedevicehandle.hpp"

SaneDeviceHandle::SaneDeviceHandle(const std::string &devName):
    paraport_(devName),
    asic_(0),
    scanner_(0)
{
    ScannerControl::switchToScanner(paraport_);
    asic_ = new A4s2600(paraport_);
    scanner_ = new ScannerControl(*asic_);
}

SaneDeviceHandle::~SaneDeviceHandle()
{
    if(asic_)
    {
        delete asic_;
    }

    if(scanner_)
    {
        delete scanner_;
    }

    ScannerControl::switchToPrinter(paraport_);
}

ParallelPortSpp &SaneDeviceHandle::getParaport()
{
    return paraport_;
}

A4s2600 &SaneDeviceHandle::getAsic()
{
    return *asic_;
}

ScannerControl &SaneDeviceHandle::getScanner()
{
    return *scanner_;
}

