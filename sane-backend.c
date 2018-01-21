#include <sane/sane.h>
#include <sane/saneopts.h>
#include <stdio.h>

enum
{
    SANE_OPTION_COUNT = 1
};

#define BACKEND_NAME se12000p
#define EXPORT(Name) _sane_se12000p_ ## Name


SANE_Status EXPORT(init) (SANE_Int * version_code, SANE_Auth_Callback authorize)
{
    printf("Called INIT!\n");

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
    static SANE_Device scanexpress =
    {
        .name="se12000p",	/* unique device name */
        .vendor="Mustek",	/* device vendor string */
        .model="SE12000P",	/* device model name */
        .type="flatbed"
    };

    static SANE_Device *list[] =
    {
        [0] = &scanexpress,
        [1] = 0
    };

    *device_list = list;

    printf("Requested Scanner List!\n");

    return SANE_STATUS_GOOD;
}

SANE_Status EXPORT(open) (SANE_String_Const name, SANE_Handle * h)
{
    printf("Open Called!! \"%s\"\n", name);
    if(h) printf("Valid h");
    if(name) printf("Valid Name");
    if(name == "") printf("Nr1");

    if(name && (name == "se12000p" || name == "") && h)
    {
        return SANE_STATUS_GOOD;
    }else {
        return SANE_STATUS_INVAL;
    }
}

void EXPORT(close) (SANE_Handle h)
{

}

const SANE_Option_Descriptor * EXPORT(get_option_descriptor) (SANE_Handle h, SANE_Int n)
{

    static SANE_Option_Descriptor options[SANE_OPTION_COUNT] =
    {
        [0]= {
            .name = SANE_NAME_NUM_OPTIONS,
            .title= SANE_TITLE_NUM_OPTIONS,
            .desc = SANE_DESC_NUM_OPTIONS,
            .type = SANE_TYPE_INT,
            .unit = SANE_UNIT_NONE,
            .size = sizeof(SANE_Int) / sizeof(SANE_Word),
            .cap = 0,
            .constraint_type = SANE_CONSTRAINT_NONE
        }
    };

    if(n>= 0 && n < SANE_OPTION_COUNT)
    {
        return &options[n];
    }

    return 0;
}

SANE_Status EXPORT(control_option) (SANE_Handle h, SANE_Int n,
                                 SANE_Action a, void *v,
                                 SANE_Int * i)
{
    if(n>=0 && n<SANE_OPTION_COUNT && v && i && h)
    {
        return SANE_STATUS_GOOD;
    }

    return SANE_STATUS_INVAL;
}

SANE_Status EXPORT(get_parameters) (SANE_Handle h,SANE_Parameters * p)
{
    if(p)
    {
        p->depth = 8;
        p->bytes_per_line = 5300;
        p->format = SANE_FRAME_GRAY;
        p->last_frame = SANE_TRUE;
        p->lines = -1;
    }

    return SANE_STATUS_INVAL;
}

SANE_Status EXPORT(start) (SANE_Handle h)
{
    return SANE_STATUS_GOOD;
}

SANE_Status EXPORT(read) (SANE_Handle h, SANE_Byte * buf, SANE_Int maxlen, SANE_Int * len)
{
    return SANE_STATUS_EOF;
}

void EXPORT(cancel) (SANE_Handle h)
{
}

SANE_Status EXPORT(set_io_mode) (SANE_Handle h, SANE_Bool m)
{
    return SANE_STATUS_GOOD;
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
