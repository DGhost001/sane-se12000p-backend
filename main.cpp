
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

        switchToPrinter();

        close(fd);
    } catch(std::exception &e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}

