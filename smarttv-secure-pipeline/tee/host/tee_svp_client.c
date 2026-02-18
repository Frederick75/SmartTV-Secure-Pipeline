#include "tee_svp_client.h"
#include <tee_client_api.h>
#include <string.h>
#include <stdlib.h>

#include "../ta/ta_svp_uuid.h"

#define CMD_IMPORT_KEYBLOB 0x0003

struct tee_svp {
    TEEC_Context ctx;
    TEEC_Session sess;
};

tee_svp_t* tee_svp_open(void)
{
    tee_svp_t* c = (tee_svp_t*)calloc(1, sizeof(*c));
    if (!c) return NULL;

    TEEC_UUID uuid = TA_SVP_UUID;
    uint32_t err_origin = 0;

    if (TEEC_InitializeContext(NULL, &c->ctx) != TEEC_SUCCESS)
        goto fail;

    if (TEEC_OpenSession(&c->ctx, &c->sess, &uuid,
                         TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin) != TEEC_SUCCESS)
        goto fail_ctx;

    return c;

fail_ctx:
    TEEC_FinalizeContext(&c->ctx);
fail:
    free(c);
    return NULL;
}

void tee_svp_close(tee_svp_t* c)
{
    if (!c) return;
    TEEC_CloseSession(&c->sess);
    TEEC_FinalizeContext(&c->ctx);
    free(c);
}

int tee_svp_import_keyblob(tee_svp_t* c, const void* blob, size_t blob_len)
{
    if (!c || !blob || blob_len == 0) return -1;

    TEEC_Operation op;
    uint32_t err_origin = 0;
    memset(&op, 0, sizeof(op));

    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
                                    TEEC_NONE, TEEC_NONE, TEEC_NONE);
    op.params[0].tmpref.buffer = (void*)blob;
    op.params[0].tmpref.size = blob_len;

    TEEC_Result r = TEEC_InvokeCommand(&c->sess, CMD_IMPORT_KEYBLOB, &op, &err_origin);
    return (r == TEEC_SUCCESS) ? 0 : -2;
}
