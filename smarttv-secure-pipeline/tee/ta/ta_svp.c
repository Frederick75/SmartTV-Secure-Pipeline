#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include "ta_svp_uuid.h"

#define CMD_OPEN_SESSION        0x0001
#define CMD_CLOSE_SESSION       0x0002
#define CMD_IMPORT_KEYBLOB      0x0003
#define CMD_DERIVE_SESSION_KEY  0x0004

TEE_Result TA_CreateEntryPoint(void) { return TEE_SUCCESS; }
void TA_DestroyEntryPoint(void) {}

TEE_Result TA_OpenSessionEntryPoint(uint32_t pt, TEE_Param p[4], void **ctx)
{
    (void)pt; (void)p;
    *ctx = (void*)0x1234;
    return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void *ctx) { (void)ctx; }

static TEE_Result cmd_import_keyblob(uint32_t pt, TEE_Param p[4])
{
    if (pt != TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
                             TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE))
        return TEE_ERROR_BAD_PARAMETERS;

    /* In production:
     * - verify authenticity
     * - bind to device unique key (HUK)
     * - seal to RPMB secure storage
     */
    (void)p;
    return TEE_SUCCESS;
}

TEE_Result TA_InvokeCommandEntryPoint(void *ctx, uint32_t cmd, uint32_t pt, TEE_Param p[4])
{
    (void)ctx;
    switch (cmd) {
    case CMD_IMPORT_KEYBLOB:
        return cmd_import_keyblob(pt, p);
    case CMD_OPEN_SESSION:
    case CMD_CLOSE_SESSION:
    case CMD_DERIVE_SESSION_KEY:
        return TEE_SUCCESS;
    default:
        return TEE_ERROR_NOT_SUPPORTED;
    }
}
