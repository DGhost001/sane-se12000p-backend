
#include <iostream>
#include <memory>

#include "parallelport.hpp"
#include "a4s2600.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/parport.h>
#include <linux/ppdev.h>
#include <sys/ioctl.h>
#include <fstream>

using namespace std;

std::shared_ptr<ParallelPortSpp> spp;
std::shared_ptr<ParallelPortEpp> epp;
std::shared_ptr<A4s2600> asic;


void switchToScanner()
{
    unsigned char sequence[]={0x15,0x95,0x35,0xB5,0x55,0xD5,0x75,0xF5,0x1,0x81};
    spp->writeString((char*)sequence,sizeof(sequence));
}

void switchToPrinter()
{
    unsigned char sequence[]={0x15,0x95,0x35,0xB5,0x55,0xD5,0x75,0xF5,0x0,0x80};
    spp->writeString((char*)sequence,sizeof(sequence));
}

static uint8_t temporaryBuffer[5300];

int main()
{
    try
    {

        int fd = open("/dev/parport0",O_RDWR);

        if(fd < 0) {
            std::error_code error(errno,std::system_category());
            throw std::system_error(error,"Failed to open /dev/parport0");
        }


        /*if(ioctl(fd,PPEXCL)<0)
        {
            std::error_code error(errno,std::system_category());
            throw std::system_error(error,"Failed to open /dev/parport0");
        }*/

        if(ioctl(fd,PPCLAIM)<0)
        {
            std::error_code error(errno,std::system_category());
            throw std::system_error(error,"Failed to open /dev/parport0");
        }


        spp = std::make_shared<ParallelPortSpp>(fd);
        spp->setupLogFile("/tmp/scanner.ols");

        switchToScanner();

        /*spp->changeMode(IEEE1284_MODE_BYTE);

        unsigned char c = 0x20;
        if(0<=write(fd,&c,1))
        {
            std::error_code error(errno,std::system_category());
            throw std::system_error(error,"Failed set EPP Mode");
        }*/



        asic = std::make_shared<A4s2600>(spp);

        cout << "ASIC Revision:"<<std::hex<<asic->getAsicRevision() <<std::endl;
        asic->resetFiFo();
        asic->getWm8144().setOperationalMode(Wm8144::Monochrom);
        asic->getWm8144().setPGAGain(Wm8144::ChannelAll,2);
        //asic->getWm8144().setPGAOffset(Wm8144::ChannelAll,-255);
        asic->getWm8144().setPixelGain(Wm8144::ChannelAll,2000);
        asic->getWm8144().setPixelOffset(Wm8144::ChannelAll,0);
        asic->enableChannel(A4s2600::AllChannels);
        asic->setCalibration(true);
        asic->selectAdFrequency(false);
        asic->setByteCount(5300);
        asic->setLowerMemoryLimit(00);
        asic->setUpperMemoryLimit(0x1ffff);
        asic->setExposureLevel(10000);
        asic->setLamp(true);
        asic->setCCDMode(true);
        asic->setDMA(true);

        std::ofstream tmp;

        tmp.open("/tmp/image.ppm");
        tmp<<"P3"<<std::endl<<"# One scaned line"<<std::endl<<sizeof(temporaryBuffer)<<" 1000"<<std::endl<<"255"<<std::endl;

        for(unsigned int i=0; i<1000; ++i)
        {
            std::cout<<"Line "<<i<<std::endl;

            asic->sendChannelData(A4s2600::Green);
            asic->waitForChannelTransferedToFiFo(A4s2600::Green);
            asic->stopChannelData(A4s2600::Green);

            asic->setDataRequest(true);
            asic->aquireImageData(temporaryBuffer,sizeof(temporaryBuffer));
            asic->setDataRequest(false);
            for(size_t j=0; j<sizeof(temporaryBuffer); ++j)
            {
                tmp<<unsigned(temporaryBuffer[j])<<" "<<unsigned(temporaryBuffer[j])<<" "<<unsigned(temporaryBuffer[j])<<" ";
            }
            tmp<<std::endl;
        }

        asic->setDMA(false);
        asic->setCCDMode(false);

        switchToPrinter();

        close(fd);



    } catch(std::exception &e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}

