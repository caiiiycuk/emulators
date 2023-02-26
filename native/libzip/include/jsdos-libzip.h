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

#ifndef EMSCRIPTEN
typedef void (*fnOnProgress) (const char *file, int extracted, int total);
void zip_set_on_progress(fnOnProgress onProgress);
#endif

ZipArchive EMSCRIPTEN_KEEPALIVE zip_from_fs(double changedAfterMs);
int EMSCRIPTEN_KEEPALIVE zip_to_fs(const char *data, uint32_t length, const char* filter);
int EMSCRIPTEN_KEEPALIVE zipfile_to_fs(const char *fileName, const char* filter);
double EMSCRIPTEN_KEEPALIVE get_changes_mtime_ms();
void EMSCRIPTEN_KEEPALIVE libzip_destroy();

int EMSCRIPTEN_KEEPALIVE zipfile_add(const char* archive, const char *file, const char *source);

#ifdef __cplusplus
}
#endif
