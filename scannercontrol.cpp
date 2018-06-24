#include "scannercontrol.hpp"
#include "parallelport.hpp"
#include "posixfifo.hpp"
#include <iostream>
#include <vector>

enum
{
    CCdWidth = 5300,
    BytePerChannel = 1,
    BytePerLine = CCdWidth * BytePerChannel
};

ScannerControl::ScannerControl(A4s2600 &asic):
    asic_(asic)
{
    initalSetupScanner();
    gotoHomePos();
    setupResolution(300);
}

void ScannerControl::gotoHomePos()
{
    asic_.setExposureLevel(10000);
    asic_.setSpeedCounter(5000); //10 Steps per clock

    asic_.setMotorDirection(A4s2600::MoveForward);
    asic_.enableMotor(true);
    asic_.enableSpeed(true);

    asic_.waitForClockChange();

    while(asic_.isAtHomePosition())
    {
        for(unsigned int i=0; i<15; ++i)
        {
            asic_.waitForClockChange(2);
            asic_.enableMove(true);
        }
    }

    asic_.enableMove(false);
    asic_.enableSpeed(false);

    asic_.setMotorDirection(A4s2600::MoveBackward);
    asic_.enableMotor(true);
    asic_.enableSpeed(true);

    while(!asic_.isAtHomePosition())
    {
        for(unsigned int i=0; i<5; ++i)
        {
            asic_.waitForClockChange(2);
            asic_.enableMove(true);
        }
    }

    asic_.setMotorDirection(A4s2600::MoveForward);
}

void ScannerControl::initalSetupScanner()
{
    std::cerr << "ASIC Revision:"<<std::hex<<asic_.getAsicRevision() <<std::endl;
    asic_.setCCDMode(false);
    asic_.setDMA(false);
    asic_.enableSync(true);
    asic_.resetFiFo();
    asic_.getWm8144().setOperationalMode(Wm8144::Monochrom);
    asic_.getWm8144().setPGAGain(Wm8144::ChannelAll,2);
    asic_.getWm8144().setPGAOffset(Wm8144::ChannelAll,127);
    asic_.getWm8144().setPixelGain(Wm8144::ChannelAll,2000);
    asic_.getWm8144().setPixelOffset(Wm8144::ChannelAll,0);
    asic_.enableChannel(A4s2600::AllChannels);
    asic_.selectAdFrequency(false);
    asic_.setByteCount(BytePerLine);
    asic_.setLowerMemoryLimit(100);
    asic_.setUpperMemoryLimit(20*BytePerLine);
    asic_.setExposureLevel(10000);
}

void ScannerControl::setupResolution(unsigned dpi)
{
    motorSpeed_ = 32500;
    switch(dpi)
    {
    case 600: multiplyer_ = 1; break;
    case 300: multiplyer_ = 2; break;
    case 200: multiplyer_ = 3; break;
    case 100: multiplyer_ = 6;break;
    case 50:  multiplyer_ = 12;break;
    default: throw std::runtime_error("Resolution "+std::to_string(dpi)+"dpi is not supported!");
    }

    motorSpeed_ = motorSpeed_ / multiplyer_;
}

int ScannerControl::getDpi()
{
    return 600 / multiplyer_;
}

unsigned ScannerControl::getBlackTotal(const Line &line)
{
    unsigned sum = 0;

    for(unsigned i = 0; i< 20; ++i)
    {
        sum += line[i];
    }

    return sum;
}

void ScannerControl::moveToStartPosition()
{
    asic_.setSpeedCounter(8125); //10 Steps per clock
    asic_.setMotorDirection(A4s2600::MoveForward);

    for(unsigned i=0; i<64; ++i)
    {
        asic_.waitForClockChange(2);
        asic_.enableMove(true);
    }

}

unsigned ScannerControl::getNumberOfLines(double sizeInCm)
{
    double sizeInInch = sizeInCm / 2.54;
    return 600 * sizeInInch / multiplyer_ ;
}

unsigned ScannerControl::getBrightSum(const Line &line)
{
    unsigned sum = 0;

    for(unsigned i = 1000; i<1200; ++i)
    {
        sum += line[i];
    }

    return sum;
}

unsigned ScannerControl::adjustAnalogOffset(A4s2600::Channel channel)
{
   Line temporaryBuffer;
   temporaryBuffer.resize(BytePerLine);

   unsigned mask = 0x80;
   unsigned offset = 0;
   unsigned min = 255*20;
   unsigned total = 0;

   while(mask!= 0x0)
   {
       unsigned newOffset = offset | mask;
       asic_.getWm8144().setPGAOffset(asic_.getWmChannel(channel),newOffset);
       scanLinesGray(channel,1,false,&temporaryBuffer[0],temporaryBuffer.size());

       total = getBlackTotal(temporaryBuffer);

       if(total > 10) //Half of all pixel are 1 the rest is 0
       {
           offset = newOffset;
       }

       if(total < min)
       {
           min = total;
       }

       mask >>= 1;
   }

   std::cerr<<" Min Black: "<<min<<std::endl;

   return offset;
}


void ScannerControl::adjustOffset(A4s2600::Channel channel)
{
    unsigned digitalOffset = 4;
    bool success = false;
    unsigned analogOffset = 0;

    while(!success)
    {
        asic_.setDigitalOffset(channel, digitalOffset);
        analogOffset = adjustAnalogOffset(channel);
        std::cerr<<"DOffset: "<<digitalOffset<<std::endl;
        if(analogOffset == 255)
        {
            digitalOffset += 4;
        }else
        {
            success = true;
        }
    }
}


void ScannerControl::adjustAnalogGain(A4s2600::Channel channel)
{
    unsigned sum;
    Line temporaryBuffer;
    temporaryBuffer.resize(BytePerLine);
    unsigned gain = 2;

    do
    {
        asic_.getWm8144().setPGAGain(asic_.getWmChannel(channel),gain);
        scanLinesGray(channel,1,false,&temporaryBuffer[0],temporaryBuffer.size());
        sum = getBrightSum(temporaryBuffer);

        if(sum<216*200)
        {
            gain+=1;
        }

    }while(sum<216*200);
}

void ScannerControl::calibrateScanner()
{
    /* Reset the Settings in the WM Controller*/
    asic_.getWm8144().setPGAGain(Wm8144::ChannelAll,2);
    asic_.getWm8144().setPGAOffset(Wm8144::ChannelAll,127);
    asic_.getWm8144().setPixelGain(Wm8144::ChannelAll,2000);
    asic_.getWm8144().setPixelOffset(Wm8144::ChannelAll,0);

    asic_.setCalibration(true);

  //  adjustOffset(A4s2600::Red); adjustAnalogGain(A4s2600::Red); adjustOffset(A4s2600::Red);
    adjustOffset(A4s2600::Green); adjustAnalogGain(A4s2600::Green); adjustOffset(A4s2600::Green);
//    adjustOffset(A4s2600::Blue); adjustAnalogGain(A4s2600::Blue); adjustOffset(A4s2600::Blue);

 //   compensatePixelNonuniformity(A4s2600::Red);
    compensatePixelNonuniformity(A4s2600::Green);
//    compensatePixelNonuniformity(A4s2600::Blue);

    asic_.setCalibration(false);
}

void ScannerControl::compensatePixelNonuniformity(A4s2600::Channel channel)
{
    Line maxBuffer;
    Line temporaryBuffer;

    maxBuffer.resize(BytePerLine);
    temporaryBuffer.resize(BytePerLine);
    for(unsigned int j=0; j<maxBuffer.size(); ++j)
    {
        maxBuffer[j] = 1;
    }

    for(unsigned int i=0; i<4; ++i)
    {
        scanLinesGray(channel,1,false,&temporaryBuffer[0],temporaryBuffer.size());

        for(unsigned int j=0; j<maxBuffer.size(); ++j)
        {
            if(temporaryBuffer[j] > maxBuffer[j])
                maxBuffer[j] = temporaryBuffer[j];
        }
    }

    for(unsigned int j=0; j<maxBuffer.size(); ++j)
    {
        perPixelGain[channel][j] = 255.0 / maxBuffer[j];
        if(perPixelGain[channel][j] > 2)
        {
            perPixelGain[channel][j] = 2.0;
        }
    }

#if 0
    for(unsigned int j=0; j<maxBuffer.size(); ++j)
    {
        unsigned int tmp = ((255 - maxBuffer[j]) * 255) / maxBuffer[j]; //Compute the gain in integer math
        if(tmp > 0xFF)
        {
            tmp = 0xFF;
        }

        maxBuffer[j] = tmp;
    }

    asic_.uploadPixelGain(channel, &maxBuffer[0], temporaryBuffer.size());
#endif
}

void ScannerControl::scanLinesGray(A4s2600::Channel channel,
                                   unsigned numberOfLines,
                                   bool moveWhileScanning,
                                   PosixFiFo &fifo,
                                   bool enableCalibration)
{
    unsigned scannedLines = 0;
    unsigned readLines = 0;
    asic_.setSpeedCounter(motorSpeed_);
    asic_.resetFiFo();

    asic_.setCCDMode(true);
    asic_.setDMA(true);

    std::vector<uint8_t> line;
    line.resize(BytePerLine);

    while(scannedLines < numberOfLines)
    {
        do
        {
            if(moveWhileScanning)
            {
                asic_.waitForClockChange(2);
            }
            asic_.sendChannelData(channel);
            if(moveWhileScanning)
            {
                asic_.enableMove(true);
            }else
            {
                asic_.waitForChannelTransferedToFiFo(channel);
            }
            ++scannedLines;
        }while(scannedLines % 20 != 0 && scannedLines < numberOfLines);

        asic_.setDataRequest(true);

        do
        {
            asic_.aquireImageData(&line[0],line.size());

            if(enableCalibration)
            {
                for(unsigned int i=0; i<5300/multiplyer_; ++i)
                {
                    double tmp = line[i*multiplyer_]*perPixelGain[channel][i*multiplyer_];
                    if(tmp >= 256)
                    {
                        tmp = 255;
                    }

                    line[i] = (uint8_t)tmp;
                }
            }

            fifo.write(&line[0], 5300/multiplyer_);

            ++readLines;
        }while(readLines != scannedLines);

        asic_.setDataRequest(false);
    }

    fifo.closeWriteFifo();

    asic_.setCCDMode(false);
    asic_.setDMA(false);

}



void ScannerControl::scanLinesGray(A4s2600::Channel channel,
                                   unsigned numberOfLines,
                                   bool moveWhileScanning,
                                   uint8_t *buffer,
                                   size_t bufferSize,
                                   bool enableCalibration)
{

    unsigned scannedLines = 0;
    unsigned readLines = 0;
    asic_.setSpeedCounter(motorSpeed_);
    asic_.resetFiFo();

    asic_.setCCDMode(true);
    asic_.setDMA(true);

    while(scannedLines < numberOfLines)
    {
        do
        {
            if(moveWhileScanning)
            {
                asic_.waitForClockChange(2);
            }
            asic_.sendChannelData(channel);
            if(moveWhileScanning)
            {
                asic_.enableMove(true);
            }else
            {
                asic_.waitForChannelTransferedToFiFo(channel);
            }
            ++scannedLines;
        }while(scannedLines % 20 != 0 && scannedLines < numberOfLines);

        asic_.setDataRequest(true);

        do
        {
            asic_.aquireImageData(buffer+BytePerLine*readLines,BytePerLine);

            if(enableCalibration)
            {
                for(unsigned int i=0; i<5300/multiplyer_; ++i)
                {
                    double tmp = buffer[BytePerLine*readLines + i*multiplyer_]*perPixelGain[channel][i*multiplyer_];
                    if(tmp >= 256)
                    {
                        tmp = 255;
                    }

                    buffer[BytePerLine*readLines + i] = (uint8_t)tmp;
                }
            }

            ++readLines;
        }while(readLines != scannedLines);

        asic_.setDataRequest(false);
    }

    asic_.setCCDMode(false);
    asic_.setDMA(false);

}

unsigned ScannerControl::getImageWidth()
{
    return 5300/multiplyer_;
}


void ScannerControl::switchToPrinter(ParallelPortBase &pb)
{
    unsigned char sequence[]={0x15,0x95,0x35,0xB5,0x55,0xD5,0x75,0xF5,0x0,0x80};
    pb.writeString(reinterpret_cast<char*>(sequence),sizeof(sequence));
}

void ScannerControl::switchToScanner(ParallelPortBase &pb)
{
    unsigned char sequence[]={0x15,0x95,0x35,0xB5,0x55,0xD5,0x75,0xF5,0x1,0x81};
    pb.writeString(reinterpret_cast<char*>(sequence),sizeof(sequence));
}
