
#include <iostream>
#include <memory>

#include "parallelport.hpp"
#include "a4s2600.hpp"
#include "scannercontrol.hpp"

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

uint8_t *image;


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



        A4s2600 asic(spp);
        ScannerControl scanner(asic);


        scanner.calibrateScanner();
        scanner.setupResolution(300);

        unsigned height = scanner.getNumberOfLines(29.7,300);
        unsigned width = scanner.getImageWidth();

        image = (uint8_t*)malloc(sizeof(uint8_t)*5300*height);


        asic.setCalibration(true);

        scanner.moveToStartPosition();

        scanner.scanLinesGray(A4s2600::Green,height,true,image,sizeof(uint8_t)*5300*height, true);

        asic.setCalibration(false);

        scanner.gotoHomePos();

        std::ofstream tmp;

        tmp.open("/tmp/image.ppm");
        tmp<<"P3"<<std::endl<<"# One scaned line"<<std::endl<<width<<" "<<height<<std::endl<<"255"<<std::endl;


        for(unsigned int i=0; i<height; ++i)
        {
            for(unsigned sl=0; sl<width; ++sl)
            {
                tmp<<unsigned(image[i*5300+sl])<<" "<<unsigned(image[i*5300+sl])<<" "<<unsigned(image[i*5300+sl])<<" ";
            }
        }

        free(image);
        tmp.close();

        switchToPrinter();

        close(fd);



    } catch(std::exception &e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}

