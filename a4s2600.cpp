#include "a4s2600.hpp"
#include "parallelport.hpp"
#include <iostream>

A4s2600::A4s2600(std::shared_ptr<ParallelPortBase> paralleport):
    parallelPort_(paralleport)
{
    readAsicRevision();
    initializeAsicIndex();

    registerMap_[12].value_ = 0x83;

    uploadConfig();


}

void A4s2600::uploadRegisterSet(const unsigned char data[], size_t elementCount)
{
    for(size_t i=0; i<elementCount; i+=2)
    {
        Register asicRegister;

        asicRegister.address_ = data[i];
        asicRegister.value_ = data[i+1];

        asicWriteRegister(asicRegister);

        for(unsigned int i=0; i<registerMap_.size(); ++i)
        {
            if(registerMap_[i].address_ == asicRegister.address_)
            {
                registerMap_[i] = asicRegister;
                break;
            }
        }
    }
}

void A4s2600::uploadConfig()
{
    if(asicRevision_ != 0xa2)
    {
        throw std::exception();
    }

    static const unsigned char initialAsicData[] =
    {   0x28, 0x0,   0x0,  0x50,    0x1,  0x28,    0x2,  0x5,    0x3,  0x18,    0x4,  0x20,
        0x5,  0x31,  0x6,  0x10,    0x7,  0x27,    0x8,  0x10,   0x9,  0x27,    0x0A, 0x10,
        0x0B, 0x27,  0x0C, 0x02/*0x83*/,    0x0D, 0x90,    0x0E, 0x40,   0x10, 0x0,     0x11, 0x0,
        0x12, 0x0,   0x16, 0x1,     0x17, 0x2C,    0x18, 0xF4 ,  0x19, 0x0,     0x1A, 0x5,
        0x27, 0x0,   0x2A, 0x0,     0x2B, 0x0,     0x2C, 0x0,    0x2D, 0x0,     0x2E, 0x0,
        0x2F, 0x0,   0x30, 0x0,     0x31, 0x0,     0x36, 0x4,    0x37, 0x0A,    0x3E, 0x4,
        0x28, 0x1
    };

    unsigned char WM8142Data1505P[] =   {    0x1, 0x17,    0x2, 0x44,    0x36, 0x0,   0x37, 0x8    };
    unsigned char WM1505P[] =           {    0x1, 0x17,    0x2, 0x4,     0x36, 0x0,   0x37, 0x8    };

    unsigned char Data1505P_8142 [] =   {    0x28, 0x0,    0x0,  0x5B,   0x28, 0x1 };
    unsigned char Data1505P [] =        {    0x28, 0x0,    0x0,  0x10,   0x28, 0x1 };


    //Setup the registers default content
    uploadRegisterSet(initialAsicData, sizeof(initialAsicData));

    registerMap_[14].value_ = 49;

    hwFeatures_ = readFromChannel(1);
    std::cout<<"Hardware Features: "<<std::hex<<static_cast<unsigned int>(hwFeatures_)<<std::endl;

    hasWM8142_ = (hwFeatures_ & 0x10) != 0;

    if(hwFeatures_ & 0x18)
    {
        if(hasWM8142_)
        {
            uploadRegisterSet(WM8142Data1505P, sizeof(WM8142Data1505P));
        }else
        {
            uploadRegisterSet(WM1505P, sizeof(WM1505P));
        }
    }

    if((hwFeatures_ & 0x3) == 0 )
    {
        if(hasWM8142_)
        {
            uploadRegisterSet(Data1505P_8142, sizeof(Data1505P_8142));
        }else
        {
            uploadRegisterSet(Data1505P, sizeof(Data1505P));
        }
    }else
    {
        if((hwFeatures_ & 0x3) == 0x2)
        {
            uploadRegisterSet(Data1505P_8142, sizeof(Data1505P_8142));
        }
    }

    if(hwFeatures_ & 4)
    {
        setAdcBitDepth(TwelveBits);
    }else
    {
        setAdcBitDepth(TenBits);
    }

    setLedMode(false);
}


void A4s2600::readAsicRevision()
{
    asicRevision_ = readFromChannel(0);
}

void A4s2600::writeToChannel(uint8_t channel, uint8_t value)
{
    //There is also a |0x18 << which seams to be used when reading back values using EPP mode ... could the 8 mean EPP? Or Tranfer?
    parallelPort_->writeByte(channel | 0x10, value);
}

uint8_t A4s2600::readFromChannel(uint8_t channel)
{
    return parallelPort_->readByte(channel | 0x98);
}

void A4s2600::asicWriteRegister(const Register &reg)
{
    writeToChannel(0x6, reg.address_);
    writeToChannel(0x5, reg.value_);

    std::cout<<"Write\t  0x06 <- 0x"<<std::hex<<int(reg.address_)<<"\t0x05 <- 0x"<<int(reg.value_)<<std::endl;
}

void A4s2600::enableChannel(enum Channel channel)
{
    uint8_t mask = 0;

    uint8_t value = registerMap_[26].value_;

    switch (channel) {
    case Red:   mask = 1<<2;    break; //011
    case Green: mask = 1<<1;    break; //101
    case Blue:  mask = 1;       break; //110
    case AllChannels: mask = 7;  break; //111
    }

    registerMap_[26].value_ = ~mask & 0x7;
    asicWriteRegister(registerMap_[26]);
}

void A4s2600::setUpperMemoryLimit(unsigned limit)
{
    registerMap_[56].value_ = limit & 0xff;
    registerMap_[57].value_ = (limit >> 8) & 0xff;
    registerMap_[58].value_ = (limit >> 16) & 0xff;

    asicWriteRegister(registerMap_[56]);
    asicWriteRegister(registerMap_[57]);
    asicWriteRegister(registerMap_[58]);
}

void A4s2600::setLowerMemoryLimit(unsigned limit)
{
    registerMap_[59].value_ = limit & 0xff;
    registerMap_[60].value_ = (limit >> 8) & 0xff;
    registerMap_[61].value_ = (limit >> 16) & 0xff;

    asicWriteRegister(registerMap_[59]);
    asicWriteRegister(registerMap_[60]);
    asicWriteRegister(registerMap_[61]);
}

void A4s2600::setAdcBitDepth(enum AdcBitDepth depth)
{
    depth_ = depth;

    switch(depth)
    {
    case TwelveBits: registerMap_[49].value_ &= 0x7f; break;
    case TenBits: registerMap_[49].value_ |= 0x80; break;
    }

    asicWriteRegister(registerMap_[49]);
}

void A4s2600::setTextThreshold(unsigned threshold)
{
    registerMap_[13].value_ = threshold & 0xff;
    asicWriteRegister(registerMap_[13]);
}

void A4s2600::setLamp(bool enabled)
{
    if(enabled)
    {
        registerMap_[16].value_ &= ~0x2; //The lamp seams to be low active ....
    }else
    {
        registerMap_[16].value_ |= 0x2;
    }

    asicWriteRegister(registerMap_[16]);
}

void A4s2600::setDataRequest(bool enabled)
{
    if(enabled)
    {
        registerMap_[16].value_ |= 0x04;
    }else
    {
        registerMap_[16].value_ &= ~0x04;
    }

    asicWriteRegister(registerMap_[16]);
}

void A4s2600::setDMA(bool enabled)
{
    if(enabled)
    {
        registerMap_[16].value_ |= 0x08;
    }else
    {
        registerMap_[16].value_ &= ~0x08;
    }

    asicWriteRegister(registerMap_[16]);
}

void A4s2600::setCCDMode(bool enabled)
{
    if(enabled)
    {
        registerMap_[16].value_ |= 0x10;
    }else
    {
        registerMap_[16].value_ &= ~0x10;
    }

    asicWriteRegister(registerMap_[16]);
}

void A4s2600::setEnableTextMode(bool enabled)
{
    if(enabled)
    {
        registerMap_[16].value_ |= 0x40;
    }else
    {
        registerMap_[16].value_ |= ~0x40;
    }

    asicWriteRegister(registerMap_[16]);
}


void A4s2600::setCalibration(bool enabled)
{
    if(enabled)
    {
        registerMap_[16].value_ |= 0x80;
    }else
    {
        registerMap_[16].value_ &= ~0x80;
    }

    asicWriteRegister(registerMap_[16]);
}

void A4s2600::initializeAsicIndex()
{
    registerMap_.resize(64); //The Asic has 64 registers mapped at different addresses based on the revision

    if(asicRevision_ == 0xa1)
    {
        unsigned int ind=0;
        for(unsigned int i=0; i<0x10; i++)
        {
            registerMap_[ind].address_ = 0x10+i;
            registerMap_[ind].value_ = 0;
            ind++;
            registerMap_[ind].address_ = 0x20+i;
            registerMap_[ind].value_ = 0;
            ind++;
            registerMap_[ind].address_ = 0x40+i;
            registerMap_[ind].value_ = 0;
            ind++;
            registerMap_[ind].address_ = 0x80+i;
            registerMap_[ind].value_ = 0;
            ind++;
        }
    }else
    {
        for(unsigned int i=0; i<0x40; i++)
        {
            registerMap_[i].address_ = i;
            registerMap_[i].value_ = 0;
        }
    }

}

void A4s2600::sendSerialClock()
{
    registerMap_[49].value_ |= 0x40;
    asicWriteRegister(registerMap_[49]);
    registerMap_[49].value_ &= ~0x40;
    asicWriteRegister(registerMap_[49]);
}

void A4s2600::selectAdFrequency(bool enable9Mhz)
{
    if(enable9Mhz)
    {
        registerMap_[49].value_ |= 0x02;
    }else
    {
        registerMap_[49].value_ &= 0xfd;
    }

    asicWriteRegister(registerMap_[49]);
}

void A4s2600::setLedMode(bool enable6Hz)
{
    if(enable6Hz)
    {
        registerMap_[12].value_ |= 0x03;
    }else
    {
        registerMap_[12].value_ &= 0xFC;
    }

    asicWriteRegister(registerMap_[12]);
}

void A4s2600::setBlackLevel(unsigned odd, unsigned even, uint8_t state)
{
    if (asicRevision_ == 0xa2 || asicRevision_ == 0xa4)
    {
        registerMap_[51].value_ = (odd >> 8) & 0xff;
        registerMap_[50].value_ = odd & 0xff;
        registerMap_[53].value_ = (odd >> 8) & 0xff;
        registerMap_[52].value_ = odd & 0xff;
        registerMap_[63].value_ = state;

        asicWriteRegister(registerMap_[51]);
        asicWriteRegister(registerMap_[50]);
        asicWriteRegister(registerMap_[53]);
        asicWriteRegister(registerMap_[52]);
        asicWriteRegister(registerMap_[63]);

    }else
    {
        registerMap_[34].value_ = (odd >> 8) & 0xff;
        registerMap_[33].value_ = odd & 0xff;
        registerMap_[20].value_ = (odd >> 8) & 0xff;
        registerMap_[19].value_ = odd & 0xff;
        registerMap_[21].value_ = state;

        asicWriteRegister(registerMap_[34]);
        asicWriteRegister(registerMap_[33]);
        asicWriteRegister(registerMap_[20]);
        asicWriteRegister(registerMap_[19]);
        asicWriteRegister(registerMap_[21]);
    }
}

void A4s2600::setByteCount(unsigned byteCount)
{
    registerMap_[22].value_ = byteCount & 0xff;
    registerMap_[23].value_ = (byteCount >> 8) & 0xff;

    asicWriteRegister(registerMap_[22]);
    asicWriteRegister(registerMap_[23]);
}

void A4s2600::resetFiFo()
{
    registerMap_[1].value_ |= 0x80;

    asicWriteRegister(registerMap_[1]);

    registerMap_[1].value_ &= ~0x80;

    asicWriteRegister(registerMap_[1]);
}

void A4s2600::writeToWMRegister(unsigned reg, unsigned value)
{
    unsigned cmd = (reg&0x2F)<<8 | (value & 0xFF);

    writeToChannel(0x0,0x0);
    writeToChannel(0x1,0x34);
    unsigned tmp = 0x2000;

    do
    {
        if(tmp & cmd)
        {
            writeToChannel(2,1);
        }else
        {
            writeToChannel(2,0);
        }

        tmp>>=1;
    }while(tmp);

    sendSerialClock(); //Commit the value
}
