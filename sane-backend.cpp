#include <sane/sane.h>
#include <sane/saneopts.h>
#include <iostream>
#include <string>
#include <functional>

#include "scannercontrol.hpp"
#include "a4s2600.hpp"
#include "parallelport.hpp"
#include "sanedevicehandle.hpp"

enum
{
    SANE_OPTION_COUNT = 3
};

struct MyOption
{
    SANE_Option_Descriptor option_;
    std::function<int(SaneDeviceHandle*, void *)> getterFunc_;
    std::function<int(SaneDeviceHandle*, void *)> setterFunc_;
};

static int setDpi(SaneDeviceHandle*, void*);
static int getDpi(SaneDeviceHandle*, void*);

static int getOptionCount(SaneDeviceHandle *, void*);


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
    .setterFunc_ = 0
};

static SANE_Int allowedDpi[] = {8,600,300,200,100,50};

static MyOption OptionDpi =
{
    .option_ = {
        .name = SANE_NAME_SCAN_RESOLUTION,
        .title= SANE_TITLE_SCAN_RESOLUTION,
        .desc = SANE_DESC_SCAN_RESOLUTION,
        .type = SANE_TYPE_INT,
        .unit = SANE_UNIT_DPI,
        .size = sizeof(SANE_Int) / sizeof(SANE_Word),
        .cap = SANE_CAP_SOFT_SELECT,
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
    .max = 300,
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
        .size = sizeof(SANE_Int) / sizeof(SANE_Word),
        .cap = SANE_CAP_SOFT_SELECT,
        .constraint_type = SANE_CONSTRAINT_RANGE,
        .constraint =
        {
            .range = &brYRange
        }
    },
    .getterFunc_ = 0,
    .setterFunc_ = 0
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
    if(version_code)
    {
        *version_code = SANE_VERSION_CODE(SANE_CURRENT_MAJOR, SANE_CURRENT_MINOR, 0);
    }

    return SANE_STATUS_GOOD;
}

void EXPORT(exit) (void)
{

}

SANE_Status EXPORT(get_devices) (const SANE_Device *** device_list,
                              SANE_Bool local_only)
{
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
                *device_list = emptyList;
            }else
            {
                std::cerr<<"Found unsupported ASIC revision: "<<asic.getAsicRevision()<<std::endl;
                *device_list = list;
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
    if(h)
    {
        SaneDeviceHandle *handle = static_cast<SaneDeviceHandle*>(h);
        delete handle;
    }
}

const SANE_Option_Descriptor * EXPORT(get_option_descriptor) (SANE_Handle h, SANE_Int n)
{

    std::cerr<<"getOption: "<<n<<std::endl;

    static SANE_Option_Descriptor options[SANE_OPTION_COUNT] =
    {
        [0]= OptionCount.option_,
        [1]= OptionDpi.option_,
        [2]= OptionBottomRightY.option_
    };

    if(n>= 0 && n < SANE_OPTION_COUNT)
    {

        return &options[n];
    }

    return 0;
}

static int setDpi(SaneDeviceHandle *handler, void *v)
{
    handler->getScanner().setupResolution(*static_cast<SANE_Int*>(v));

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

SANE_Status EXPORT(control_option) (SANE_Handle h, SANE_Int n,
                                 SANE_Action a, void *v,
                                 SANE_Int * i)
{
    MyOption availableOptions[SANE_OPTION_COUNT] =
    {
        [0]= OptionCount,
        [1]= OptionDpi,
        [2]= OptionBottomRightY
    };

    if(n>=0 && n<SANE_OPTION_COUNT && v && h)
    {
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

                return SANE_STATUS_GOOD;
            }else if(a == SANE_ACTION_SET_VALUE && availableOptions[n].setterFunc_)
            {
                SANE_Int result = availableOptions[n].setterFunc_(handle, v);
                if(i)
                {
                    *i = result;
                }

                return SANE_STATUS_GOOD;
            }
        }catch(const std::exception &e)
        {
            std::cerr<<e.what()<<std::endl;
            return SANE_STATUS_IO_ERROR;
        }
    }

    return SANE_STATUS_INVAL;
}

SANE_Status EXPORT(get_parameters) (SANE_Handle h,SANE_Parameters * p)
{
    if(p && h)
    {
        SaneDeviceHandle *handle = static_cast<SaneDeviceHandle*>(h);

        try
        {
            p->depth = 8;
            p->bytes_per_line = handle->getScanner().getImageWidth();
            p->format = SANE_FRAME_GRAY;
            p->last_frame = SANE_TRUE;
            p->lines = -1;
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
    if(h)
    {
        SaneDeviceHandle *handle = static_cast<SaneDeviceHandle*>(h);

        try
        {
            if(handle->isScanFinished() && handle->copyFinished())
            {
                return SANE_STATUS_EOF;
            }

            //In case of blocking io ... wait for the scanner to finish
            if(!handle->isScanFinished() && handle->getBlocking())
            {
                handle->waitForFinishedScan();
            }else if(!handle->isScanFinished()) //Do we have actual image data??
            {
                if(*len)
                {
                    *len = 0;
                }

                return SANE_STATUS_GOOD;
            }

            int result = handle->copyImagebuffer(buf,maxlen);
            if(len)
            {
                *len = result;
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
}

SANE_Status EXPORT(set_io_mode) (SANE_Handle h, SANE_Bool m)
{
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
    return SANE_STATUS_UNSUPPORTED;
}

SANE_String_Const EXPORT(strstatus) (SANE_Status status)
{
    static SANE_String_Const txt = "Not implemented yet";

    return txt;
}
