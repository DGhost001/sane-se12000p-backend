#include <sane/sane.h>
#include <sane/saneopts.h>
#include <iostream>
#include <string>
#include <functional>
#include <string.h>

#include "scannercontrol.hpp"
#include "a4s2600.hpp"
#include "parallelport.hpp"
#include "sanedevicehandle.hpp"

enum
{
    SANE_OPTION_COUNT = 7
};

struct MyOption
{
    SANE_Option_Descriptor option_;
    std::function<int(SaneDeviceHandle*, void *)> getterFunc_;
    std::function<int(SaneDeviceHandle*, void *)> setterFunc_;
    SANE_Int value;
};

static int setDpi(SaneDeviceHandle*, void*);
static int getDpi(SaneDeviceHandle*, void*);

static int getOptionCount(SaneDeviceHandle *, void*);

static int getScanHeight(SaneDeviceHandle *, void*);
static int setScanHeight(SaneDeviceHandle *, void*);

static int getScanWidth(SaneDeviceHandle *, void*);
static int setScanWidth(SaneDeviceHandle *, void*);

static int getStart(SaneDeviceHandle *, void*);
static int setStart(SaneDeviceHandle *, void*);

static int getScanMode(SaneDeviceHandle *, void*);
static int setScanMode(SaneDeviceHandle *, void*);


static MyOption OptionCount =
{
    .option_ = {
        .name = SANE_NAME_NUM_OPTIONS,
        .title= SANE_TITLE_NUM_OPTIONS,
        .desc = SANE_DESC_NUM_OPTIONS,
        .type = SANE_TYPE_INT,
        .unit = SANE_UNIT_NONE,
        .size = sizeof(SANE_Int) / sizeof(SANE_Word),
        .cap = 0,
        .constraint_type = SANE_CONSTRAINT_NONE
    },
    .getterFunc_ = getOptionCount,
    .setterFunc_ = 0,
    .value = SANE_OPTION_COUNT
};

static SANE_Int allowedDpi[] = {5,600,300,200,100,50};

static MyOption OptionDpi =
{
    .option_ = {
        .name = SANE_NAME_SCAN_RESOLUTION,
        .title= SANE_TITLE_SCAN_RESOLUTION,
        .desc = SANE_DESC_SCAN_RESOLUTION,
        .type = SANE_TYPE_INT,
        .unit = SANE_UNIT_DPI,
        .size = sizeof(SANE_Int),
        .cap = SANE_CAP_SOFT_SELECT | SANE_CAP_SOFT_DETECT,
        .constraint_type = SANE_CONSTRAINT_WORD_LIST,
        .constraint =
        {
            .word_list = allowedDpi
        }
    },
    .getterFunc_ = getDpi,
    .setterFunc_ = setDpi
};

static SANE_Range brYRange =
{
    .min = 0,
    .max = 297,
    .quant = 0
};

static MyOption OptionBottomRightY =
{
    .option_ = {
        .name = SANE_NAME_SCAN_BR_Y,
        .title= SANE_TITLE_SCAN_BR_Y,
        .desc = SANE_DESC_SCAN_BR_Y,
        .type = SANE_TYPE_INT,
        .unit = SANE_UNIT_MM,
        .size = sizeof(SANE_Int),
        .cap = SANE_CAP_SOFT_SELECT | SANE_CAP_SOFT_DETECT,
        .constraint_type = SANE_CONSTRAINT_RANGE,
        .constraint =
        {
            .range = &brYRange
        }
    },
    .getterFunc_ = getScanHeight,
    .setterFunc_ = setScanHeight
};

static SANE_Range brXRange =
{
    .min = 0,
    .max = 225,
    .quant = 0
};

static MyOption OptionBottomRightX =
{
    .option_ = {
        .name = SANE_NAME_SCAN_BR_X,
        .title= SANE_TITLE_SCAN_BR_X,
        .desc = SANE_DESC_SCAN_BR_X,
        .type = SANE_TYPE_INT,
        .unit = SANE_UNIT_MM,
        .size = sizeof(SANE_Int),
        .cap = SANE_CAP_SOFT_SELECT | SANE_CAP_SOFT_DETECT,
        .constraint_type = SANE_CONSTRAINT_RANGE,
        .constraint =
        {
            .range = &brXRange
        }
    },
    .getterFunc_ = getScanWidth,
    .setterFunc_ = setScanWidth
};

static SANE_Range startXRange =
{
    .min = 0,
    .max = 225,
    .quant = 0
};


static MyOption OptionStartX =
{
    .option_ = {
        .name = SANE_NAME_SCAN_TL_X,
        .title= SANE_TITLE_SCAN_TL_X,
        .desc = SANE_DESC_SCAN_TL_X,
        .type = SANE_TYPE_INT,
        .unit = SANE_UNIT_MM,
        .size = sizeof(SANE_Int),
        .cap = SANE_CAP_SOFT_SELECT | SANE_CAP_SOFT_DETECT,
        .constraint_type = SANE_CONSTRAINT_RANGE,
        .constraint =
        {
            .range = &startXRange
        }
    },
    .getterFunc_ = getStart,
    .setterFunc_ = setStart
};

static SANE_Range startYRange =
{
    .min = 0,
    .max = 297,
    .quant = 0
};


static MyOption OptionStartY =
{
    .option_ = {
        .name = SANE_NAME_SCAN_TL_Y,
        .title= SANE_TITLE_SCAN_TL_Y,
        .desc = SANE_DESC_SCAN_TL_Y,
        .type = SANE_TYPE_INT,
        .unit = SANE_UNIT_MM,
        .size = sizeof(SANE_Int),
        .cap = SANE_CAP_SOFT_SELECT | SANE_CAP_SOFT_DETECT,
        .constraint_type = SANE_CONSTRAINT_RANGE,
        .constraint =
        {
            .range = &startYRange
        }
    },
    .getterFunc_ = getStart,
    .setterFunc_ = setStart
};

static SANE_String_Const mode_list[] = {
    SANE_VALUE_SCAN_MODE_LINEART,
    SANE_VALUE_SCAN_MODE_GRAY,
    SANE_VALUE_SCAN_MODE_COLOR
};

static MyOption OptionScanMode =
{
    .option_ = {
        .name = SANE_NAME_SCAN_MODE,
        .title= SANE_TITLE_SCAN_MODE,
        .desc = SANE_DESC_SCAN_MODE,
        .type = SANE_TYPE_STRING,
        .unit = SANE_UNIT_NONE,
        .size = 64,
        .cap = SANE_CAP_SOFT_SELECT | SANE_CAP_SOFT_DETECT,
        .constraint_type = SANE_CONSTRAINT_STRING_LIST,
        .constraint =
        {
            .string_list = mode_list
        }
    },
    .getterFunc_ = getScanMode,
    .setterFunc_ = setScanMode
};

#define BACKEND_NAME se12000p
#define EXPORT(Name) _sane_se12000p_ ## Name

/*
 * For the API to work, the functions must be exported as "C" functions.
 * This is excactly what it is done below.
 **/
#ifdef __cplusplus
extern "C" {
#endif
    SANE_Status EXPORT(init) (SANE_Int * version_code, SANE_Auth_Callback authorize);
    void EXPORT(exit) (void);
    SANE_Status EXPORT(get_devices) (const SANE_Device *** device_list, SANE_Bool local_only);
    SANE_Status EXPORT(open) (SANE_String_Const name, SANE_Handle * h);
    void EXPORT(close) (SANE_Handle h);
    const SANE_Option_Descriptor * EXPORT(get_option_descriptor) (SANE_Handle h, SANE_Int n);
    SANE_Status EXPORT(control_option) (SANE_Handle h, SANE_Int n, SANE_Action a, void *v, SANE_Int * i);
    SANE_Status EXPORT(get_parameters) (SANE_Handle h,SANE_Parameters * p);
    SANE_Status EXPORT(start) (SANE_Handle h);
    SANE_Status EXPORT(read) (SANE_Handle h, SANE_Byte * buf, SANE_Int maxlen, SANE_Int * len);
    void EXPORT(cancel) (SANE_Handle h);
    SANE_Status EXPORT(set_io_mode) (SANE_Handle h, SANE_Bool m);
    SANE_Status EXPORT(get_select_fd) (SANE_Handle h, SANE_Int *fd);
    SANE_String_Const EXPORT(strstatus) (SANE_Status status);
#ifdef __cplusplus
}
#endif

SANE_Status EXPORT(init) (SANE_Int * version_code, SANE_Auth_Callback authorize)
{

    std::cerr<<"init"<<std::endl;

    if(version_code)
    {
        *version_code = SANE_VERSION_CODE(SANE_CURRENT_MAJOR, SANE_CURRENT_MINOR, 0);
    }

    return SANE_STATUS_GOOD;
}

void EXPORT(exit) (void)
{
    std::cerr<<"exit "<<std::endl;
}

SANE_Status EXPORT(get_devices) (const SANE_Device *** device_list,
                              SANE_Bool local_only)
{
    std::cerr<<"getdevice "<<std::endl;
    static SANE_Device const scanexpress =
    {
        .name="se12000p",	/* unique device name */
        .vendor="Mustek",	/* device vendor string */
        .model="SE12000P",	/* device model name */
        .type="flatbed scanner"
    };

    static SANE_Device const* list[] =
    {
        [0] = &scanexpress,
        [1] = 0
    };

    static SANE_Device const* emptyList[]=
    {
        [0] = 0
    };

    *device_list = emptyList; //Assume Error in the first place and prove otherwise

    try
    {
        ParallelPortSpp spp("/dev/parport0");

        ScannerControl::switchToScanner(spp);
        try
        {
            A4s2600 asic(spp);

            if(asic.getAsicRevision()== 0xa2)
            {
                *device_list = list;
            }else
            {
                std::cerr<<"Found unsupported ASIC revision: "<<asic.getAsicRevision()<<std::endl;
            }
        }catch(const std::exception &e)
        {
            std::cerr<<e.what()<<std::endl;
        }

        ScannerControl::switchToPrinter(spp);
    }catch(const std::exception &e)
    {
        std::cerr<<e.what()<<std::endl;
    }

    return SANE_STATUS_GOOD;
}

SANE_Status EXPORT(open) (SANE_String_Const name, SANE_Handle * h)
{
    std::cerr<<"open "<<std::endl;
    //The name pointer and the handle pointer must be valid, else
    //someone is using the API in a wrong way ...
    if(!name || !h)
    {
        return SANE_STATUS_INVAL;
    }

    std::string devName(name);

    //Only react if the name is empty or the supported scanner is mentioned
    if(devName == "" || devName == "se12000p")
    {
        try
        {
            *h = new SaneDeviceHandle("/dev/parport0"); //It is not good to hardcode this ... but for now it will work
        }catch(const std::exception &e)
        {
            std::cerr<<e.what()<<std::endl;
            return SANE_STATUS_IO_ERROR;
        }

        return SANE_STATUS_GOOD;
    }


    //If we got down here ... we do not support the requested scanner
    return SANE_STATUS_INVAL;
}

void EXPORT(close) (SANE_Handle h)
{
    std::cerr<<"close "<<std::endl;
    if(h)
    {
        SaneDeviceHandle *handle = static_cast<SaneDeviceHandle*>(h);
        delete handle;
    }
}

const SANE_Option_Descriptor * EXPORT(get_option_descriptor) (SANE_Handle /*h*/, SANE_Int n)
{

    std::cerr<<"getOptionDescr: "<<n<<std::endl;

    static SANE_Option_Descriptor options[SANE_OPTION_COUNT] =
    {
        [0]= OptionCount.option_,
        [1]= OptionDpi.option_,
        [2]= OptionBottomRightX.option_,
        [3]= OptionBottomRightY.option_,
        [4]= OptionStartX.option_,
        [5]= OptionStartY.option_,
        [6]= OptionScanMode.option_
    };

    if(n>= 0 && n < SANE_OPTION_COUNT)
    {

        return &options[n];
    }

    return 0;
}

static int setDpi(SaneDeviceHandle *handler, void *v)
{
    SANE_Int i = *static_cast<SANE_Int*>(v);

    if(i == 0)
    {
        handler->getScanner().setupResolution(300);
        return SANE_INFO_RELOAD_PARAMS | SANE_INFO_INEXACT;
    }else
    {
        handler->getScanner().setupResolution(i);
    }

    return SANE_INFO_RELOAD_PARAMS;
}

static int getDpi(SaneDeviceHandle *handler, void *v)
{
    *static_cast<SANE_Int*>(v) = handler->getScanner().getDpi();

    return 0;
}

static int getOptionCount(SaneDeviceHandle *, void *v)
{
    *static_cast<SANE_Int*>(v) = SANE_OPTION_COUNT;
    return 0;
}

static int getScanHeight(SaneDeviceHandle *handle, void* v)
{
    *static_cast<SANE_Int*>(v) = handle->getImageHeightInCm() * 10;
    return 0;
}

static int setScanHeight(SaneDeviceHandle *handle, void *v)
{
    double i = *static_cast<SANE_Int*>(v)/10.0;

    if(i>0 && i<29.8)
    {
        handle->setImageHeightInCm(i);
    }else
    {
        handle->setImageHeightInCm(29.7);
        return SANE_INFO_RELOAD_PARAMS | SANE_INFO_INEXACT;
    }

    return SANE_INFO_RELOAD_PARAMS;
}

static int getScanWidth(SaneDeviceHandle *, void *v)
{
    *static_cast<SANE_Int*>(v) = 225;
    return 0;
}

static int getStart(SaneDeviceHandle *, void* v)
{
    *static_cast<SANE_Int*>(v) = 0;
    return 0;

}

static int setStart(SaneDeviceHandle *, void*)
{
    return SANE_INFO_RELOAD_PARAMS | SANE_INFO_INEXACT;
}



static int setScanWidth(SaneDeviceHandle *, void*)
{
    return SANE_INFO_RELOAD_PARAMS | SANE_INFO_INEXACT;
}


static int getScanMode(SaneDeviceHandle *, void* v)
{
   memcpy(v,&mode_list[1], strlen(mode_list[1]));
   return 0;
}

static int setScanMode(SaneDeviceHandle *, void*)
{
   return SANE_INFO_RELOAD_PARAMS | SANE_INFO_INEXACT;
}


SANE_Status EXPORT(control_option) (SANE_Handle h, SANE_Int n,
                                 SANE_Action a, void *v,
                                 SANE_Int * i)
{
    std::cerr<<"ctrl"<<n<<" "<<a<<std::endl;
    static MyOption availableOptions[SANE_OPTION_COUNT] =
    {
        [0]= OptionCount,
        [1]= OptionDpi,
        [2]= OptionBottomRightX,
        [3]= OptionBottomRightY,
        [4]= OptionStartX,
        [5]= OptionStartY,
        [6]= OptionScanMode
    };

    if(n>=0 && n<SANE_OPTION_COUNT && v && h)
    {
        if(i)
        {
            *i = 0;
        }

        SaneDeviceHandle *handle = static_cast<SaneDeviceHandle*>(h);

        try
        {
            if(a == SANE_ACTION_GET_VALUE && availableOptions[n].getterFunc_)
            {

                 SANE_Int result = availableOptions[n].getterFunc_(handle, v);
                if(i)
                {
                    *i = result;
                }

                std::cerr<<std::dec<<"ctrl ok "<<*static_cast<SANE_Int*>(v)<<std::endl;
                return SANE_STATUS_GOOD;
            }else if(a == SANE_ACTION_SET_VALUE && availableOptions[n].setterFunc_)
            {

                SANE_Int result = availableOptions[n].setterFunc_(handle, v);
                if(i)
                {
                    *i = result;
                }

                std::cerr<<std::dec<<"ctrl ok "<<*static_cast<SANE_Int*>(v)<<" value="<<availableOptions[n].value<<std::endl;
                return SANE_STATUS_GOOD;
            }
        }catch(const std::exception &e)
        {
            std::cerr<<e.what()<<std::endl;
            return SANE_STATUS_IO_ERROR;
        }
    }
    std::cerr<<"ctrl err"<<std::endl;
    return SANE_STATUS_INVAL;
}

SANE_Status EXPORT(get_parameters) (SANE_Handle h,SANE_Parameters * p)
{
    std::cerr<<"param"<<std::endl;
    if(p && h)
    {
        SaneDeviceHandle *handle = static_cast<SaneDeviceHandle*>(h);

        std::cerr<<"Width:"<<handle->getScanner().getImageWidth()<<std::endl;

        try
        {
            p->depth = 8;
            p->bytes_per_line = handle->getScanner().getImageWidth();
            p->pixels_per_line = handle->getScanner().getImageWidth();
            p->format = SANE_FRAME_GRAY;
            p->last_frame = SANE_TRUE;
            p->lines = handle->getScanner().getNumberOfLines(handle->getImageHeightInCm());
        }catch(const std::exception &e)
        {
            std::cerr<<e.what()<<std::endl;
            return SANE_STATUS_IO_ERROR;
        }

        return SANE_STATUS_GOOD;
    }

    return SANE_STATUS_INVAL;
}

SANE_Status EXPORT(start) (SANE_Handle h)
{
    std::cerr<<"start"<<std::endl;
    if(h)
    {
        try
        {
            SaneDeviceHandle *handle = static_cast<SaneDeviceHandle*>(h);
            handle->startScanning();
        }catch(const std::exception &e)
        {
            std::cerr<<e.what()<<std::endl;
            return SANE_STATUS_IO_ERROR;
        }

        return SANE_STATUS_GOOD;
    }else
    {
        return SANE_STATUS_INVAL;
    }
}

SANE_Status EXPORT(read) (SANE_Handle h, SANE_Byte * buf, SANE_Int maxlen, SANE_Int * len)
{
    std::cerr<<"read"<<std::endl;
    if(h)
    {
        SaneDeviceHandle *handle = static_cast<SaneDeviceHandle*>(h);

        if(len)
        {
           *len = 0;
        }

        try
        {

            size_t bytesRead = handle->copyImagebuffer(buf, maxlen);

            if(bytesRead == 0 && handle->isScanFinished())
            {
                return SANE_STATUS_EOF;
            }

            if(len)
            {
                *len = bytesRead;
            }

            return SANE_STATUS_GOOD;

        }catch(const std::exception &e)
        {
            std::cerr<<e.what()<<std::endl;
            return SANE_STATUS_IO_ERROR;
        }
    }

    return SANE_STATUS_INVAL;

}

void EXPORT(cancel) (SANE_Handle h)
{
    std::cerr<<"cancle"<<std::endl;
}

SANE_Status EXPORT(set_io_mode) (SANE_Handle h, SANE_Bool m)
{
    std::cerr<<"set_io_mode"<<std::endl;
    if(h)
    {
        SaneDeviceHandle *handle = static_cast<SaneDeviceHandle*>(h);
        handle->setBlocking(m == SANE_TRUE);

        return SANE_STATUS_GOOD;
    }

    return SANE_STATUS_INVAL;
}

SANE_Status EXPORT(get_select_fd) (SANE_Handle h, SANE_Int *fd)
{
    std::cerr<<"get_select"<<std::endl;
    return SANE_STATUS_UNSUPPORTED;
}

SANE_String_Const EXPORT(strstatus) (SANE_Status status)
{
    std::cerr<<"status"<<std::endl;

    static SANE_String_Const txt = "Not implemented yet";

    return txt;
}
