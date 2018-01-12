#ifndef PARALLELPORT_H
#define PARALLELPORT_H

#include <stdint.h>
#include <stddef.h>
#include <string>

class ParallelPortBase
{
public:
    ParallelPortBase(int fd): fd_(fd) {}
    virtual ~ParallelPortBase(){}

    virtual void writeByte(char address, char byte) = 0;
    virtual char readByte(char address) = 0;
    virtual void readString(char address, char * const buffer, size_t bufferSize) = 0;
    virtual void writeString(char address, char const*const buffer, size_t bfferSize) = 0;

    virtual void writeByte(char byte) = 0;
    virtual char readByte() = 0;
    virtual void readString( char * const buffer, size_t bufferSize) = 0;
    virtual void writeString( char const*const buffer, size_t bfferSize) = 0;

    void changeMode(int mode);

protected:
    int fd_;
    void execAndCheck(int retValue, const std::string &message);
    void execAndCheck(bool retValue, const std::string &message);
};

/* ------------------------------------------------------------------------------------------*/

class ParallelPortEpp: public ParallelPortBase
{
public:
    ParallelPortEpp(int fd);

    virtual void writeByte(char address, char byte);
    virtual char readByte(char address);
    virtual void readString(char address, char * const buffer, size_t bufferSize);
    virtual void writeString(char address, char const*const buffer, size_t bfferSize);
};

/* ------------------------------------------------------------------------------------------*/

class ParallelPortSpp: public ParallelPortBase
{
public:
    ParallelPortSpp(int fd);

    virtual void writeByte(char addr, char byte);
    virtual char readByte(char);
    virtual void readString(char, char * const buffer, size_t bufferSize);
    virtual void writeString(char address, char const*const buffer, size_t bufferSize);

    virtual void writeByte(char byte);
    virtual char readByte();
    virtual void readString( char * const buffer, size_t bufferSize);
    virtual void writeString(char const*const buffer, size_t bufferSize);

};

#endif // PARALLELPORT_H
