#include "parallelport.hpp"

#include <sys/ioctl.h>
#include <stdio.h>
#include <linux/parport.h>
#include <linux/ppdev.h>
#include <exception>
#include <errno.h>
#include <string.h>
#include <system_error>
#include <unistd.h>


void ParallelPortBase::changeMode(int mode)
{
    execAndCheck(ioctl(fd_, PPNEGOT, &mode), "PP Modechage failed");
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


ParallelPortEpp::ParallelPortEpp(int fd): ParallelPortBase(fd)
{
}

char ParallelPortEpp::readByte(char address)
{
    changeMode(IEEE1284_MODE_EPP | IEEE1284_ADDR);
    execAndCheck(write(fd_,&address, sizeof(address)) == sizeof(address), "Write to PP failed");
    changeMode(IEEE1284_MODE_EPP | IEEE1284_DATA);
    char result;
    execAndCheck(read(fd_,&result, sizeof(result)) == sizeof(result), "Read from PP failed");

    return result;
}

void ParallelPortEpp::readString(char address, char * const buffer, size_t bufferSize)
{
    changeMode(IEEE1284_MODE_EPP | IEEE1284_ADDR);
    execAndCheck(write(fd_,&address, sizeof(address)) == sizeof(address), "Write to PP failed");
    changeMode(IEEE1284_MODE_EPP | IEEE1284_DATA);

    char *bp = buffer;
    for(unsigned int i = bufferSize; i>0; --i,++bp )
    {
        execAndCheck(read(fd_,bp, 1) == 1, "Read from PP failed");
    }
}

void ParallelPortEpp::writeByte(char address, char byte)
{
    changeMode(IEEE1284_MODE_EPP | IEEE1284_ADDR);
    execAndCheck(write(fd_,&address, sizeof(address)) == sizeof(address), "Write to PP failed");
    changeMode(IEEE1284_MODE_EPP | IEEE1284_DATA);

    execAndCheck(write(fd_,&byte, sizeof(byte)) == sizeof(byte), "Read from PP failed");
}

void ParallelPortEpp::writeString(char address, char const * const buffer, size_t bufferSize)
{
    changeMode(IEEE1284_MODE_EPP | IEEE1284_ADDR);
    execAndCheck(write(fd_,&address, sizeof(address)) == sizeof(address), "Write to PP failed");
    changeMode(IEEE1284_MODE_EPP | IEEE1284_DATA);

    char const* bp = buffer;
    for(unsigned int i = bufferSize; i>0; --i,++bp )
    {
        execAndCheck(write(fd_,bp, 1) == 1, "Read from PP failed");
    }
}

void ParallelPortSpp::writeByte(char, char byte)
{
    changeMode(IEEE1284_MODE_COMPAT);
    execAndCheck(write(fd_,&byte, sizeof(byte)) == sizeof(byte), "Read from PP failed");

}

char ParallelPortSpp::readByte(char)
{
    changeMode(IEEE1284_MODE_COMPAT);
    char result;
    execAndCheck(read(fd_,&result, sizeof(result)) == sizeof(result), "Read from PP failed");

    return result;
}

void ParallelPortSpp::readString(char, char * const buffer, size_t bufferSize)
{
    changeMode(IEEE1284_MODE_COMPAT);

    char *bp = buffer;
    for(unsigned int i = bufferSize; i>0; --i,++bp )
    {
        execAndCheck(read(fd_,bp, 1) == 1, "Read from PP failed");
    }
}

void ParallelPortSpp::writeString(char, char const*const buffer, size_t bufferSize)
{
    changeMode(IEEE1284_MODE_COMPAT);
    char const* bp = buffer;
    for(unsigned int i = bufferSize; i>0; --i,++bp )
    {
        execAndCheck(write(fd_,bp, 1) == 1, "Read from PP failed");
    }

}
