// macro to add function name to kernel messages
#undef pr_fmt
#define pr_fmt(fmt) "%s:"fmt,__func__

struct pcdev_platform_data {
    int size;
    int permission;
    const char *serial_number;
};

enum permissions {READONLY = 0x1, WRITEONLY = 0x2, READWRITE = 0x3};
