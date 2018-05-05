#include "parallelport.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <linux/parport.h>
#include <linux/ppdev.h>
#include <exception>
#include <errno.h>
#include <string.h>
#include <system_error>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <ios>

typedef std::chrono::duration<uint64_t, std::ratio<1,1000000> > UsDuration;

static void udelay(unsigned useconds)
{

    auto start = std::chrono::high_resolution_clock::now();
    UsDuration spawn;
    do
    {
        spawn = std::chrono::duration_cast<UsDuration>(std::chrono::high_resolution_clock::now() - start);
    }while(spawn.count()<useconds);
}

ParallelPortBase::ParallelPortBase(int fd):
    fd_(fd)
{
    execAndCheck(fd_, "Failed to open parallel port");
    execAndCheck(ioctl(fd,PPCLAIM),"Failed to claim the parallel port");
}

ParallelPortBase::~ParallelPortBase()
{
    close(fd_);
}

void ParallelPortBase::changeMode(int mode)
{
    execAndCheck(ioctl(fd_, PPSETMODE, &mode), "PP Modechage failed "+std::to_string(mode));
}

void ParallelPortBase::execAndCheck(bool retValue, const std::string &message)
{
    if(!retValue)
    {
        std::error_code error(errno,std::system_category());
        throw std::system_error(error,message);
    }
}

void ParallelPortBase::execAndCheck(int retValue, const std::__cxx11::string &message)
{
    execAndCheck(retValue>=0, message);
}


void ParallelPortBase::setupLogFile(const std::string &filename)
{
    logfile_.open(filename,std::ios_base::trunc | std::ios_base::out);
    logfile_<<";Rate: 1000000"<<std::endl;
    logfile_<<";Channels: 32"<<std::endl;
}

void ParallelPortBase::startLogging()
{
    logStartTime_ = std::chrono::high_resolution_clock::now();
    isLogging_ = true;
}

void ParallelPortBase::stopLogging()
{
    isLogging_ = false;
}

void ParallelPortBase::logRead(char address, char data)
{
    if(isLogging_)
    {
        uint32_t output = (uint32_t(address)&0xFF) << 8 | (uint32_t(data)&0xFF);

        logfile_<<std::hex<<output<<"@";
        logfile_<<std::dec<<std::chrono::duration_cast<UsDuration>(std::chrono::high_resolution_clock::now() - logStartTime_).count()<<std::endl;
    }
}

void ParallelPortBase::logWrite(char address, char data)
{
    if(isLogging_)
    {
        uint32_t output = 0x10000 | (uint32_t(address)&0xFF) << 8 | (uint32_t(data)&0xFF);
        logfile_<<std::hex<<output<<"@";
        logfile_<<std::dec<<std::chrono::duration_cast<UsDuration>(std::chrono::high_resolution_clock::now() - logStartTime_).count()<<std::endl;
    }
}


ParallelPortEpp::ParallelPortEpp(int fd): ParallelPortBase(fd)
{
}

char ParallelPortEpp::readByte(char address)
{
    changeMode(IEEE1284_MODE_EPP | IEEE1284_ADDR);
    execAndCheck(write(fd_,&address, sizeof(address)) == sizeof(address), "Write to PP failed");
    changeMode(IEEE1284_MODE_EPP | IEEE1284_DATA);
    char result;
    execAndCheck(read(fd_,&result, sizeof(result)) == sizeof(result), "EPP Read from PP failed");

    return result;
}

void ParallelPortEpp::readString(char address, char * const buffer, size_t bufferSize)
{
    changeMode(IEEE1284_MODE_EPP | IEEE1284_ADDR);
    execAndCheck(write(fd_,&address, sizeof(address)) == sizeof(address), "EPP Write to PP failed");
    changeMode(IEEE1284_MODE_EPP | IEEE1284_DATA);

    char *bp = buffer;
    for(size_t i = bufferSize; i>0; --i,++bp )
    {
        execAndCheck(read(fd_,bp, 1) == 1, "EPP Read from PP failed");
    }
}

void ParallelPortEpp::writeByte(char address, char byte)
{
    changeMode(IEEE1284_MODE_EPP | IEEE1284_ADDR);
    execAndCheck(write(fd_,&address, sizeof(address)) == sizeof(address), "Write to PP failed");
    changeMode(IEEE1284_MODE_EPP | IEEE1284_DATA);

    execAndCheck(write(fd_,&byte, sizeof(byte)) == sizeof(byte), "EPP Write to PP failed");
}

void ParallelPortEpp::writeString(char address, char const * const buffer, size_t bufferSize)
{
    changeMode(IEEE1284_MODE_EPP | IEEE1284_ADDR);
    execAndCheck(write(fd_,&address, sizeof(address)) == sizeof(address), "Write to PP failed");
    changeMode(IEEE1284_MODE_EPP | IEEE1284_DATA);

    char const* bp = buffer;
    for(size_t i = bufferSize; i>0; --i,++bp )
    {
        execAndCheck(write(fd_,bp, 1) == 1, "EPP write to PP failed");
    }
}


ParallelPortSpp::ParallelPortSpp(const std::string &device): ParallelPortBase(open(device.c_str(),O_RDWR))
{
}

void ParallelPortSpp::writeByte(char addr, char byte)
{
    const unsigned char  low = 0x04;
    const unsigned char ahigh= 0x06;
    const unsigned char dhigh= 0x05;

    changeMode(IEEE1284_MODE_COMPAT);

    writeByte(addr);
    udelay(1);
    execAndCheck(ioctl(fd_,PPWCONTROL,&low), "W-Low-1");
    udelay(1);
    execAndCheck(ioctl(fd_,PPWCONTROL,&ahigh), "W-High-2");
    udelay(4);
    execAndCheck(ioctl(fd_,PPWCONTROL,&low), "W-Low-3");
    udelay(1);
    writeByte(byte);
    udelay(4);
    execAndCheck(ioctl(fd_,PPWCONTROL,&dhigh), "W-High-4");
    udelay(1);
    execAndCheck(ioctl(fd_,PPWCONTROL,&low), "W-Low-5");
    udelay(4);

    logWrite(addr, byte);
}

char ParallelPortSpp::readByte(char addr)
{

    const unsigned char olow = 0x04;
    const unsigned char ohigh= 0x06;
    const unsigned char ilow = 0x04;//0x84;
    const unsigned char ihigh= 0x05;//0x85;
    const int data_input = 1;
    const int data_output = 0;

    char result;

    changeMode(IEEE1284_MODE_COMPAT);

    writeByte(addr);
    udelay(1);
    execAndCheck(ioctl(fd_,PPWCONTROL,&olow), "R-Low-1");
    udelay(1);
    execAndCheck(ioctl(fd_,PPWCONTROL,&ohigh), "R-High-2");
    udelay(4);
    execAndCheck(ioctl(fd_,PPWCONTROL,&olow), "R-Low-3");
    udelay(1);
    execAndCheck(ioctl(fd_,PPDATADIR,&data_input), "R-Low-3");
    execAndCheck(ioctl(fd_,PPWCONTROL,&ihigh), "R-Low-4");
    udelay(4);
    result = readByte();
    execAndCheck(ioctl(fd_,PPWCONTROL,&ilow), "R-Low-5");
    udelay(1);
    execAndCheck(ioctl(fd_,PPDATADIR,&data_output), "R-Low-3");
    execAndCheck(ioctl(fd_,PPWCONTROL,&olow), "R-Low-6");
    udelay(1);

    logRead(addr, result);

    return result;
}

void ParallelPortSpp::readString(char addr, char * const buffer, size_t bufferSize)
{
    const unsigned char olow = 0x04;
    const unsigned char ohigh= 0x06;
    const unsigned char ilow = 0x04;
    const unsigned char ihigh= 0x05;

    const int data_input = 1;
    const int data_output = 0;

    changeMode(IEEE1284_MODE_COMPAT);

    writeByte(addr);
    udelay(1);
    execAndCheck(ioctl(fd_,PPWCONTROL,&olow), "R-Low-1");
    udelay(1);
    execAndCheck(ioctl(fd_,PPWCONTROL,&ohigh), "R-High-2");
    udelay(4);
    execAndCheck(ioctl(fd_,PPWCONTROL,&olow), "R-Low-3");
    udelay(1);

    execAndCheck(ioctl(fd_,PPDATADIR,&data_input), "R-Low-3");

    for(size_t i = 0; i<bufferSize; ++i)
    {
        execAndCheck(ioctl(fd_,PPWCONTROL,&ihigh), "R-Low-4");
        udelay(1);
        buffer[i] = readByte();
        execAndCheck(ioctl(fd_,PPWCONTROL,&ilow), "R-Low-5");
        udelay(1);
        logRead(addr, buffer[i]);
    }

    execAndCheck(ioctl(fd_,PPDATADIR,&data_output), "R-Low-3");

    execAndCheck(ioctl(fd_,PPWCONTROL,&olow), "R-Low-6");
    udelay(1);
}

void ParallelPortSpp::writeString(char addr, char const*const buffer, size_t bufferSize)
{
    const unsigned char low = 0x04;
    const unsigned char ahigh= 0x06;
    const unsigned char dhigh= 0x06;

    changeMode(IEEE1284_MODE_COMPAT);

    writeByte(addr);
    udelay(1);
    execAndCheck(ioctl(fd_,PPWCONTROL,&low), "W-Low-1");
    udelay(1);
    execAndCheck(ioctl(fd_,PPWCONTROL,&ahigh), "W-High-2");
    udelay(4);
    execAndCheck(ioctl(fd_,PPWCONTROL,&low), "W-Low-3");
    udelay(1);

    for(size_t i = 0; i<bufferSize; ++i )
    {
        writeByte(buffer[i]);
        udelay(4);
        execAndCheck(ioctl(fd_,PPWCONTROL,&dhigh), "W-High-4");
        udelay(1);
        execAndCheck(ioctl(fd_,PPWCONTROL,&low), "W-Low-5");
        udelay(4);

        logWrite(addr, buffer[i]);
    }
}

void  ParallelPortSpp::writeByte(char byte)
{
    const unsigned char c = static_cast<unsigned char>(byte);

    changeMode(IEEE1284_MODE_COMPAT);
    execAndCheck(ioctl(fd_,PPWDATA,&c), "Write to PP failed");
}

char  ParallelPortSpp::readByte()
{
    unsigned char c = 0;

    changeMode(IEEE1284_MODE_COMPAT);

    execAndCheck(ioctl(fd_,PPRDATA,&c), "Write to PP failed");

    return c;
}

void  ParallelPortSpp::readString( char * const buffer, size_t bufferSize)
{
    for(size_t i=0; i<bufferSize; ++i)
    {
        buffer[i] = readByte();
    }
}

void  ParallelPortSpp::writeString( char const*const buffer, size_t bufferSize)
{
    for(size_t i=0; i<bufferSize; ++i)
    {
        writeByte(buffer[i]);
    }
}

