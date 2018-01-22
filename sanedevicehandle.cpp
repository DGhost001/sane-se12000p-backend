#include "sanedevicehandle.hpp"
#include <functional>

SaneDeviceHandle::SaneDeviceHandle(const std::string &devName):
    paraport_(devName),
    asic_(0),
    scanner_(0),
    image(0),
    bytesAvailable_(0),
    bytesRead_(0),
    imageHeightInCm_(29.7),
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

    if(image)
    {
        free(image);
    }

    if(thread_)
    {
        if(thread_->joinable())
        {
            thread_->join();
        }
        delete thread_;
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

    bytesAvailable_ = 0;
    bytesRead_ = 0;
    scanFinished_ = false;
    thread_ = new std::thread(std::bind(&SaneDeviceHandle::runScan,this));
}

void SaneDeviceHandle::runScan()
{
    unsigned dpi = scanner_->getDpi(); //Normally not required, but it is safer to assume that this is the best way to do it
    scanner_->calibrateScanner();
    scanner_->setupResolution(dpi);

    asic_->setCalibration(true); //The only way that the image is currently correctly acquired ... at least until the integrated image processing works

    scanner_->moveToStartPosition();

    unsigned height = scanner_->getNumberOfLines(imageHeightInCm_);
    unsigned width = scanner_->getImageWidth();

    image = (uint8_t*)malloc(sizeof(uint8_t)*5300*height); //We always need a image width of 5300 !! For the readout of the CCD!!

    if(!image)
    {
        return;
    }

    scanner_->scanLinesGray(A4s2600::Green,height,true,image,sizeof(uint8_t)*5300*height, true);

    asic_->setCalibration(false);

    scanner_->gotoHomePos();

    bytesAvailable_ = height * width * sizeof(uint8_t);
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
    if(scanFinished_)
    {
        size_t thisBuffSize = 0;
        unsigned width = scanner_->getImageWidth();
        for(;bytesAvailable_!=bytesRead_ && thisBuffSize < bufferLength;++bytesRead_, ++thisBuffSize)
        {
            *buff = image[(bytesRead_ / width)*5300 + bytesRead_ % width];
        }

        return thisBuffSize;
    }

    return 0;
}

bool SaneDeviceHandle::getBlocking() const
{
    return blocking_;
}

void SaneDeviceHandle::setBlocking(bool blocking)
{
    blocking_ = blocking;
}
