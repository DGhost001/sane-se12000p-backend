#include "posixfifo.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string>

#include <system_error>

PosixFiFo::PosixFiFo()
{
    remove("/tmp/fifo_se12000p");
    if(mkfifo("/tmp/fifo_se12000p", S_IRWXU | S_IRWXG | S_IRWXO))
    {
        std::string message = "Failed to create FiFo";
        std::error_code error(errno,std::system_category());
        throw std::system_error(error,message);
    }

    int tmp = open("/tmp/fifo_se12000p",O_APPEND | O_RDONLY |O_NONBLOCK );
    fdWrite_ = open("/tmp/fifo_se12000p",O_APPEND | O_WRONLY | O_NONBLOCK);
    fdRead_ = open("/tmp/fifo_se12000p",O_APPEND | O_RDONLY );
    close(tmp);
}

PosixFiFo::~PosixFiFo()
{
    closeReadFifo();
    closeWriteFifo();
    remove("/tmp/fifo_se12000p");
}

void PosixFiFo::write(uint8_t *buffer, size_t bufferSize)
{
    while(bufferSize > 0)
    {
        int byteWritten = ::write(fdWrite_,buffer,bufferSize);

        if(byteWritten > 0)
        {
           bufferSize -= byteWritten;
           buffer += byteWritten;
        }
        if(byteWritten < 0)
        {
            std::string message = "Failed to write to FiFo";
            std::error_code error(errno,std::system_category());
            throw std::system_error(error,message);
        }
    }
}

size_t PosixFiFo::read(uint8_t *buffer, size_t bufferSize)
{
    int bytesRead = ::read(fdRead_,buffer, bufferSize);

    if(bytesRead<0)
    {
        std::string message = "Failed to read from FiFo";
        std::error_code error(errno,std::system_category());
        throw std::system_error(error,message);
    }

    return bytesRead;
}

void PosixFiFo::closeWriteFifo()
{
    if(fdWrite_)
    {
        close(fdWrite_);
    }
}


void PosixFiFo::closeReadFifo()
{
    if(fdRead_)
    {
        close(fdRead_);
    }
}
