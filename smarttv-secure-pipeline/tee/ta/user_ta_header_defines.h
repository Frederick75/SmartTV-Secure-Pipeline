#ifndef USER_TA_HEADER_DEFINES_H
#define USER_TA_HEADER_DEFINES_H

#include "ta_svp_uuid.h"

#define TA_UUID TA_SVP_UUID
#define TA_FLAGS (TA_FLAG_SINGLE_INSTANCE | TA_FLAG_MULTI_SESSION)
#define TA_STACK_SIZE (2 * 1024)
#define TA_DATA_SIZE  (32 * 1024)

#define TA_DESCRIPTION "SVP Key/Session Manager (scaffold)"
#define TA_VERSION "0.1"

#endif
