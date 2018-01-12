#ifndef A4S2600_H
#define A4S2600_H

#include <memory>
#include <stdint.h>
#include <vector>

#include "register.hpp"

class ParallelPortBase;


class A4s2600
{
public:
    struct Register
    {
        unsigned int address_;
        uint8_t value_;
    };

    enum ColorModes
    {
        Text,
        Gray,
        Rgb
    };

    enum AdcBitDepth
    {
        TwelveBits,
        TenBits
    };

    enum Channel
    {
        Red,
        Green,
        Blue,
        AllChannels
    };

    enum
    {
        lastOnChipMemoryAddress = 0x1FFFF //129kbyte on chip memory
    };


    A4s2600(std::shared_ptr<ParallelPortBase> paralleport);

    void setUpperMemoryLimit(unsigned limit);
    void setLowerMemoryLimit(unsigned limit);
    void setAdcBitDepth(enum AdcBitDepth depth);
    void enableChannel(enum Channel channel);

    enum AdcBitDepth getAdcBitDepth() {return depth_;}

    void setTextThreshold(unsigned threshold);

    /* Register 14 controls */
    void setCCDMode(bool enabled);
    void setDMA(bool enabled);
    void setDataRequest(bool enabled);
    void setEnableTextMode(bool enabled);
    void setCalibration(bool enabled);
    void setLamp(bool enabled);

    void sendSerialClock();
    void selectAdFrequency(bool enable9Mhz);

    void setByteCount(unsigned byteCount);

    void resetFiFo();

    void setLedMode(bool enable6Hz);

    void setBlackLevel(unsigned odd, unsigned even, uint8_t state);

    unsigned getAsicRevision() { return asicRevision_; }

    void writeToWMRegister(unsigned reg, unsigned value);

private:
    std::shared_ptr<ParallelPortBase> parallelPort_;
    std::vector<Register> registerMap_;

    AdcBitDepth depth_;
    unsigned int asicRevision_;
    unsigned hwFeatures_;

    bool hasWM8142_;


    void asicWriteRegister(const Register &reg);
    void writeToChannel(uint8_t channel, uint8_t value);
    uint8_t readFromChannel(uint8_t channel);

    void readAsicRevision();
    void initializeAsicIndex();
    void uploadConfig();

    void uploadRegisterSet(const unsigned char data[], size_t elementCount);

};

#endif // A4S2600_H
