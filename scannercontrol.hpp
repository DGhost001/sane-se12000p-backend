#ifndef SCANNERCONTROL_H
#define SCANNERCONTROL_H

#include "a4s2600.hpp"

class ScannerControl
{
public:  

    ScannerControl(A4s2600 &asic);

    void gotoHomePos();
    void setupResolution(unsigned dpi);
    void scanLinesGray(A4s2600::Channel channel, unsigned numberOfLines, bool moveWhileScanning, uint8_t *buffer, size_t bufferSize, bool enableCalibration = false);
    void calibrateScanner();
    void moveToStartPosition();
    unsigned getNumberOfLines(double sizeInCm, unsigned dpi);
    unsigned getImageWidth();

    static void switchToScanner(ParallelPortBase &pb);
    static void switchToPrinter(ParallelPortBase &pb);

private:
    typedef std::vector<uint8_t> Line;
    A4s2600 &asic_;
    unsigned motorSpeed_;
    unsigned multiplyer_;
    double perPixelGain[3][5300];

    void initalSetupScanner();
    void adjustAnalogGain(A4s2600::Channel channel);
    void adjustOffset(A4s2600::Channel channel);
    unsigned adjustAnalogOffset(A4s2600::Channel channel);
    unsigned getBlackTotal(const Line &line);
    unsigned getBrightSum(const Line &line);
    void compensatePixelNonuniformity(A4s2600::Channel channel);
};

#endif // SCANNERCONTROL_H
