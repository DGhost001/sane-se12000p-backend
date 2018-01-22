
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
uint8_t *image;


int main()
{
    try
    {

        spp = std::make_shared<ParallelPortSpp>("/dev/parport0");

        ScannerControl::switchToScanner(*spp);

        A4s2600 asic(*spp);
        ScannerControl scanner(asic);


        scanner.calibrateScanner();
        scanner.setupResolution(300);

        unsigned height = scanner.getNumberOfLines(29.7);
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

        ScannerControl::switchToPrinter(*spp);





    } catch(std::exception &e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}

