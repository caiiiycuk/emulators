//
// Created by caiiiycuk on 14.04.2020.
//

#ifdef EMSCRIPTEN
#include <emscripten.h>
#else
#include <jsdos-libzip.h>
#endif

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
#ifdef EMSCRIPTEN
    emscripten_exit_with_live_runtime();
#else
    ZipArchive archive = zip_from_fs(0);
    if (!archive) {
      return -1;
    }

    uint32_t length = ((uint32_t*) archive)[0];
    char* data = ((char*) archive + sizeof(uint32_t));

    FILE* f = fopen("/tmp/archive.zip", "wb");
    fwrite(data, sizeof(char), length, f);
    fclose(f);

    free(archive);

    printf("Archive size %.2f Kb\n", length / 1024.0f);
#endif
    return 0;
}
