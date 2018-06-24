#ifndef POSIXFIFO_H
#define POSIXFIFO_H

#include <stdint.h>
#include <stddef.h>

class PosixFiFo
{
public:
    PosixFiFo();
    ~PosixFiFo();

    void write(uint8_t *buffer, size_t bufferSize);
    size_t read(uint8_t *buffer, size_t bufferSize);

    void closeWriteFifo();
    void closeReadFifo();

private:
    int fdRead_; ///< This is the linux read file descriptor
    int fdWrite_; ///< This is the linux write file descriptor

};

#endif // POSIXFIFO_H
