#ifndef WM8144_H
#define WM8144_H

class A4s2600;

class Wm8144
{
public:

    enum ColorChannels
    {
        ChannelRed = 0,
        ChannelGreen = 1,
        ChannelBlue = 2,
        ChannelAll = 3
    };

    enum OperationalMode
    {
        Color,
        Monochrom
    } ;

    Wm8144(A4s2600 &asic);

    void setOperationalMode(OperationalMode mode);

    void setPGAGain(ColorChannels channel, unsigned gain);
    void setPGAOffset(ColorChannels channel, int offset);
    void setPixelOffset(ColorChannels channel, unsigned offset);
    void setPixelGain(ColorChannels channel, unsigned gain);


private:
    A4s2600 &asic_;

};

#endif // WM8144_H
