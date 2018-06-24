#include "sanedevicehandle.hpp"
#include <functional>
#include <iostream>
#include <unistd.h>

SaneDeviceHandle::SaneDeviceHandle(const std::string &devName):
    fifo_(nullptr),
    paraport_(devName),
    asic_(nullptr),
    scanner_(nullptr),
    thread_(nullptr),
    bytesAvailable_(0),
    bytesRead_(0),
    imageHeightInCm_(5),
    scanFinished_(true),
    blocking_(true)
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

    if(thread_)
    {
        if(thread_->joinable())
        {
            thread_->join();
        }
        delete thread_;
    }

    if(fifo_)
    {
        delete fifo_;
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

void SaneDeviceHandle::startScanning()
{
    if(thread_ && !scanFinished_)
    {
        throw std::runtime_error("There is currently a scan ongoing ... can't start a new one");
    }

    if(thread_)
    {
        if(thread_->joinable())
        {
            thread_->join();
        }
        delete thread_;
    }

    if(fifo_)
    {
        delete fifo_;
    }

    bytesAvailable_ = 0;
    bytesRead_ = 0;
    scanFinished_ = false;

    unsigned dpi = scanner_->getDpi(); //Normally not required, but it is safer to assume that this is the best way to do it
    scanner_->calibrateScanner();
    scanner_->setupResolution(dpi);

    asic_->setCalibration(true); //The only way that the image is currently correctly acquired ... at least until the integrated image processing works

    scanner_->moveToStartPosition();

    fifo_ = new PosixFiFo(); //Create a new Fifo

    thread_ = new std::thread(std::bind(&SaneDeviceHandle::runScan,this));
}

void SaneDeviceHandle::runScan()
{
    unsigned height = scanner_->getNumberOfLines(imageHeightInCm_);
    unsigned width = scanner_->getImageWidth();

    scanner_->scanLinesGray(A4s2600::Green,height,true,*fifo_, true);

    asic_->setCalibration(false);

    scanner_->gotoHomePos();

    bytesAvailable_ = height * width * sizeof(uint8_t);

    std::cerr<<std::dec<<"Finished image "<<width<<"x"<<height<< " ("<<bytesAvailable_<<" bytes)"<<std::endl;
    scanFinished_ = true;
}

void SaneDeviceHandle::waitForFinishedScan()
{
    if(thread_)
    {
        thread_->join();
    }
}


size_t SaneDeviceHandle::copyImagebuffer(uint8_t *buff, size_t bufferLength)
{
    if(!fifo_)
    {
        throw std::exception();
    }
    return fifo_->read(buff, bufferLength);
}

bool SaneDeviceHandle::getBlocking() const
{
    return blocking_;
}

void SaneDeviceHandle::setBlocking(bool blocking)
{
    blocking_ = blocking;
}

double SaneDeviceHandle::getImageHeightInCm() const
{
    return imageHeightInCm_;
}

void SaneDeviceHandle::setImageHeightInCm(double imageHeightInCm)
{
    imageHeightInCm_ = imageHeightInCm;
}
