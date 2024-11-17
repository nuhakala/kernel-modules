#include "linux/ioctl.h"

#define TM_GET _IOR('N', 666, char *)
#define TM_START _IO('N', 667)
#define TM_STOP _IO('N', 668)
#define TM_PID _IOW('N', 669, int)
