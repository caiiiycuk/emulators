#ifdef EMSCRIPTEN
#include <emscripten.h>
#else
#ifndef EMSCRIPTEN_KEEPALIVE
#define EEMSCRIPTEN_KEEPALIVE /* EMSCRIPTE_KEEPALIVE */
#endif
#endif

#include <stdint.h>

// <uint32_t:length><char*:data>
typedef void* ZipArchive;

#ifdef __cplusplus
extern "C" {
#endif

ZipArchive EMSCRIPTEN_KEEPALIVE zip_from_fs(double changedAfterMs);
int EMSCRIPTEN_KEEPALIVE zip_to_fs(const char *data, uint32_t length);
int EMSCRIPTEN_KEEPALIVE zipfile_to_fs(const char *fileName);
double EMSCRIPTEN_KEEPALIVE get_changes_mtime_ms();
void EMSCRIPTEN_KEEPALIVE libzip_destroy();

#ifdef __cplusplus
}
#endif
