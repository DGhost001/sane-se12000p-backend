#include "wm8144.hpp"
#include "a4s2600.hpp"

Wm8144::Wm8144(A4s2600 &asic):
    asic_(asic)
{
}

void Wm8144::setOperationalMode(OperationalMode mode)
{
    if(mode == Color)
    {
        asic_.writeToWMRegister(0x1, 0x1B); // ENADC, CDS, DEFPG, DEFPO
        asic_.writeToWMRegister(0x2, 0x04); // INVOP
        asic_.writeToWMRegister(0x3, 0xE2); // RLC = 2 (Clamp at 3.5V), CHAN= 3 (Monochrom channel dont care), CDSREF=2 (-1 Clk), PWP=0
        asic_.writeToWMRegister(0x5, 0x10); // Mode12
        asic_.writeToWMRegister(0x2B, 0x02);// ALL PGA Gains = 2
        asic_.writeToWMRegister(0x27, 0x00);// DAC Sign = "+"
    }else
    {
        asic_.writeToWMRegister(0x1, 0x1F); // ENADC, CDS, DEFPG, DEFPO, MONO
        asic_.writeToWMRegister(0x2, 0x04); // INVOP
        asic_.writeToWMRegister(0x3, 0x62); // RLC = 2 (Clamp at 3.5V), CHAN= 01 (Green), CDSREF=2 (-1 Clk), PWP=0
        asic_.writeToWMRegister(0x5, 0x10); // Mode12
        asic_.writeToWMRegister(0x2B, 0x02);// ALL PGA Gains = 2
        asic_.writeToWMRegister(0x27, 0x00);// DAC Sign = "+"
    }
}

void Wm8144::setPGAGain(ColorChannels channel, unsigned gain)
{
    asic_.writeToWMRegister(0b101000 | channel, gain & 0x1F);
}

void Wm8144::setPGAOffset(ColorChannels channel, int offset)
{
    const unsigned poffset = abs(offset);

    asic_.writeToWMRegister(0b100000 | channel, poffset & 0xFF);
    asic_.writeToWMRegister(0b100100 | channel, offset > 0? 0:1);
}

void Wm8144::setPixelOffset(ColorChannels channel, unsigned offset)
{
    asic_.writeToWMRegister(0b101100 | channel, offset & 0x3F);
}

void Wm8144::setPixelGain(ColorChannels channel, unsigned gain)
{
    asic_.writeToWMRegister(0b110000 | channel, (gain >>4)& 0xFF);
    asic_.writeToWMRegister(0b110100 | channel, gain & 0x0F);
}
