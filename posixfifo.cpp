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
    int fds[2];

    if(pipe(fds))
    {
        std::string message = "Failed to create a pipe";
        std::error_code error(errno,std::system_category());
        throw std::system_error(error,message);
    }

    fdRead_ = fds[0];
    fdWrite_= fds[1];
}

PosixFiFo::~PosixFiFo()
{
    closeReadFifo();
    closeWriteFifo();
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
        fdWrite_ = 0;
    }
}


void PosixFiFo::closeReadFifo()
{
    if(fdRead_)
    {
        close(fdRead_);
        fdRead_ = 0;
    }
}
