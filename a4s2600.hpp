#ifndef A4S2600_H
#define A4S2600_H

#include <memory>
#include <stdint.h>
#include <vector>

#include "register.hpp"
#include "wm8144.hpp"

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
        Red = 0,
        Green = 1,
        Blue = 2,
        AllChannels = 3
    };

    enum
    {
        lastOnChipMemoryAddress = 0x1FFFF //129kbyte on chip memory
    };

    enum MotorDirection
    {
        MoveForward,
        MoveBackward
    };

    A4s2600(std::shared_ptr<ParallelPortBase> paralleport);

    void setUpperMemoryLimit(unsigned limit);
    void setLowerMemoryLimit(unsigned limit);
    void setAdcBitDepth(AdcBitDepth depth);
    void enableChannel(Channel channel);

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

    unsigned readBlackLevel();

    unsigned setDigitalOffset(Channel channel, unsigned offset);

    void enableSerial(bool enable);
    void writeToWMRegister(unsigned reg, unsigned value);

    /**
     * @brief setExposureLevel set the exposure time in us of the CCD
     * @param level This is the exposure time in us
     *
     * @note After some experimentation with this, it looks like the exposure time is
     * given in clock ticks and one clock tick is equal to 1us, which means that the scanner seams to have
     * a internal operationl clock of 1Mhz.
     *
     * The clock exposed on channel 6 is directly influenced by this parameter. In the scanner everything seams to be
     * triggered on the rising edge of the clock pulse.
     */

    void setExposureLevel(unsigned level);

    unsigned getStatus(); //Return the raw status ...

    unsigned getCurrentExposureLevel(); //Return the current exposure level

    void waitForClockPulse();
    void waitForClockPulse(unsigned count) { for(unsigned i=0; i<count; ++i) waitForClockPulse(); }
    void waitForClockLevel(bool high);
    void waitForClockChange();
    void waitForClockChange(unsigned count) { for(unsigned i=0; i<count; ++i) waitForClockChange(); }
    bool getClockLevel();

    void waitForChannelTransferedToFiFo(Channel channel);
    bool fifoAboveLowerLimit();
    bool fifoAboveUpperLimit();

    void sendChannelData(Channel channel);
    void stopChannelData(Channel channel);

    bool isAtHomePosition();

    void setMotorDirection(MotorDirection direction);
    void enableMotor(bool enabled);
    void enableSpeed(bool enable);
    void enableMove(bool enable);
    void setSpeedCounter(unsigned counter);
    void enableSync(bool enable);


    Wm8144 &getWm8144() { return wm8144_; }

    void aquireImageData(uint8_t *buffer, size_t bufferSize);

    Wm8144::ColorChannels getWmChannel(Channel channel);

    void uploadPixelGain(Channel channel, uint8_t *buffer, size_t bufferSize);

private:
    std::shared_ptr<ParallelPortBase> parallelPort_;
    std::vector<Register> registerMap_;

    AdcBitDepth depth_;
    unsigned int asicRevision_;
    unsigned hwFeatures_;
    unsigned motorControlAndChannelSelection_;

    bool hasWM8142_;


    Wm8144 wm8144_;

    void asicWriteRegister(const Register &reg);
    void writeToChannel(uint8_t channel, uint8_t value);
    uint8_t readFromChannel(uint8_t channel);
    void readBufferFromChannel(uint8_t channel, uint8_t *, size_t);

    void readAsicRevision();
    void initializeAsicIndex();
    void uploadConfig();

    void uploadRegisterSet(const unsigned char data[], size_t elementCount);

};

#endif // A4S2600_H
