#pragma once
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct tee_svp tee_svp_t;

tee_svp_t* tee_svp_open(void);
void tee_svp_close(tee_svp_t* c);
int tee_svp_import_keyblob(tee_svp_t* c, const void* blob, size_t blob_len);

#ifdef __cplusplus
}
#endif
