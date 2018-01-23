#ifndef SANEDEVICEHANDLE_H
#define SANEDEVICEHANDLE_H

#include "parallelport.hpp"
#include "a4s2600.hpp"
#include "scannercontrol.hpp"

#include <thread>

class SaneDeviceHandle
{
public:
    SaneDeviceHandle(const std::string &devName);
    ~SaneDeviceHandle();

    ParallelPortSpp& getParaport();
    A4s2600& getAsic();
    ScannerControl& getScanner();

    void startScanning();
    size_t copyImagebuffer(uint8_t *buff, size_t bufferLength);
    bool copyFinished() const { return bytesAvailable_ == bytesRead_; }
    bool isScanFinished() const { return scanFinished_; }

    void waitForFinishedScan();

    bool getBlocking() const;
    void setBlocking(bool blocking);

    double getImageHeightInCm() const;
    void setImageHeightInCm(double imageHeightInCm);

private:
    ParallelPortSpp paraport_;
    A4s2600 *asic_;
    ScannerControl *scanner_;
    std::thread *thread_;
    uint8_t *image;
    size_t bytesAvailable_;
    size_t bytesRead_;
    double imageHeightInCm_;
    bool scanFinished_;
    bool blocking_;

    void runScan();
};

#endif // SANEDEVICEHANDLE_H
