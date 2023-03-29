#include <zip.h>
#include <jsdos-libzip.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>

#include <sys/stat.h>

#ifdef EMSCRIPTEN
EM_JS(double, emsc_getMTimeMs, (const char* path), {
  var lookup = FS.lookupPath(UTF8ToString(path));
  return lookup.node.timestamp;
});

EM_JS(void, emsc_progress, (const char* file, int32_t extracted, int32_t count), {
  if (Module.libzip_progress !== undefined) {
    Module.libzip_progress(UTF8ToString(file), extracted, count);
  }
});
#endif

const char *libzipTempArchive = "libzip-temp-archive.zip";

#ifndef EMSCRIPTEN
fnOnProgress onProgress = 0;
void zip_set_on_progress(fnOnProgress newOnProgress) {
    onProgress = newOnProgress;
}
#endif

ZipArchive readZipArchiveFile(const char *path) {
    FILE *file;
    char *buffer;
    long length;

    file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "zip_from_fs: can't open file file: '%s', errno: %d\n", path, errno);
        return 0;
    }

    fseek(file, 0, SEEK_END);
    length = ftell(file);
    rewind(file);

    buffer = (char *) malloc(length * sizeof(char) + sizeof(uint32_t));
    fread(buffer + sizeof(uint32_t), length, 1, file);
    fclose(file);
    ((uint32_t*) buffer)[0] = (uint32_t) length;
    return (ZipArchive) buffer;
}

void safe_create_dir(const char *dir) {
#ifdef __MINGW32__
    if (mkdir(dir) < 0) {
#else
    if (mkdir(dir, 0755) < 0) {
#endif
        if (errno != EEXIST) {
            perror(dir);
            exit(1);
        }
    }
}

void ensure_parent_dir(const char* filename) {
  char* copy = strdup(filename);
  char* dir = dirname(copy);
  if (strlen(dir) > 1) {
    safe_create_dir(dir);
  }
  free(copy);
}

static int is_dir(const char *dir) {
    struct stat st;
    stat(dir, &st);
    return S_ISDIR(st.st_mode);
}

static double getMTimeMs(const char* file) {
    struct stat fileStat;
#ifdef EMSCRIPTEN
    double mTimeMs = emsc_getMTimeMs(file);
#else
    if (stat(file, &fileStat) == -1) {
        fprintf(stderr, "zip_from_fs: getMTimeMs can't stat file %s\n", file);
        return 0;
    }
#if defined(WIN32) || defined(__APPLE__)
    double mTimeMs = (double) fileStat.st_mtime * 1000;
#else
    double mTimeMs = (double) fileStat.st_mtim.tv_sec * 1000 + (double) fileStat.st_mtim.tv_nsec / 1000000;
#endif
#endif

    return mTimeMs;
}

double lastExtractedMTimeMs = 0;
double EMSCRIPTEN_KEEPALIVE get_changes_mtime_ms() {
    return lastExtractedMTimeMs;
}

int zip_recursively(zip_t *zipArchive, const char *directory, double changedAfterMs) {
    struct dirent *dirp;

    DIR *dp = opendir(directory);
    if (dp == 0) {
        fprintf(stderr, "zip_from_fs: can't open directory %s\n", directory);
        return 0;
    }

    while ((dirp = readdir(dp)) != 0) {
        if (strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 &&
            strcmp(dirp->d_name, libzipTempArchive) != 0) {
            int nameLength = strlen(directory) + strlen(dirp->d_name) + 2;
            char *nameInFs = (char*) malloc(nameLength);
            strcpy(nameInFs, directory);
            strcat(nameInFs, "/");
            strcat(nameInFs, dirp->d_name);
            char *nameInArchive = nameInFs + 2;
            if (is_dir(nameInFs)) {
                if (changedAfterMs > 0 && strcmp(".jsdos", nameInArchive) == 0) {
                    continue;
                }
                if (zip_dir_add(zipArchive, nameInArchive, ZIP_FL_ENC_UTF_8) == -1) {
                    fprintf(stderr, "zip_from_fs: can't create directory %s, cause %s\n", nameInFs,
                            zip_strerror(zipArchive));
                    return 0;
                }
                if (!zip_recursively(zipArchive, nameInFs, changedAfterMs)) {
                    return 0;
                }
            } else {
                if (changedAfterMs > 0) {
                  if (getMTimeMs(nameInFs) <= changedAfterMs) {
                    free(nameInFs);
                    continue;
                  }
                }

                zip_source_t *source = zip_source_file(zipArchive, nameInArchive, 0, 0);
                if (source == 0) {
                    fprintf(stderr, "zip_from_fs: can't create file %s, cause %s\n", nameInFs,
                            zip_strerror(zipArchive));
                    return 0;
                }
                int index = zip_file_add(zipArchive, nameInArchive, source, ZIP_FL_ENC_UTF_8);
                if (index == -1) {
                    zip_source_free(source);
                    fprintf(stderr, "zip_from_fs: can't create file %s, cause %s\n", nameInFs,
                            zip_strerror(zipArchive));
                    return 0;
                } else {
                    if (zip_set_file_compression(zipArchive, index, ZIP_CM_STORE, 0) == -1) {
                        fprintf(stderr, "zip_from_fs: can't set compression level for %s, cause %s\n", nameInFs,
                                zip_strerror(zipArchive));
                        return 0;
                    }
                }
            }
            free(nameInFs);
        }
    }
    closedir(dp);
    return 1;
}

ZipArchive zip_fs(double changedAfterMs) {
    struct zip *zipArchive;
    char buf[100];
    int err;

    if ((zipArchive = zip_open(libzipTempArchive, ZIP_CREATE | ZIP_TRUNCATE, &err)) == NULL) {
        zip_error_to_str(buf, sizeof(buf), err, errno);
        fprintf(stderr, "zip_from_fs: can't open zip archive: %s\n", buf);
        return 0;
    }


    int success = zip_recursively(zipArchive, ".", changedAfterMs);
    if (zip_close(zipArchive) == -1) {
        fprintf(stderr, "zip_from_fs: can't close zip archive %s\n", zip_strerror(zipArchive));
        return 0;
    }

    if (!success) {
        return 0;
    }

    if (chmod(libzipTempArchive, S_IRWXU | S_IRWXG | S_IRWXO) != 0) {
        fprintf(stderr, "zip_from_fs : unable to set read mode for archive\n");
    }

    ZipArchive archive = readZipArchiveFile(libzipTempArchive);

    if (remove(libzipTempArchive) != 0) {
        fprintf(stderr, "zip_from_fs: unable to delete archive\n");
    }

    return archive;
}

ZipArchive EMSCRIPTEN_KEEPALIVE zip_from_fs(double changedAfterMs) {
  return zip_fs(changedAfterMs);
}

int EMSCRIPTEN_KEEPALIVE zip_to_fs(const char *data, uint32_t length, const char *filter) {
    FILE *archive = fopen(libzipTempArchive, "wb");
    if (!archive) {
        fprintf(stderr, "zip_to_fs: unable to create archive file\n");
        return 1;
    }
    fwrite(data, length, 1, archive);
    fclose(archive);

    zipfile_to_fs(libzipTempArchive, filter);

    if (remove(libzipTempArchive) != 0) {
        fprintf(stderr, "zip_to_fs: unable to delete archive\n");
        return 1;
    }

    return 0;
}

#define ZIPTOFS_BUFFER_SIZE 4096

int EMSCRIPTEN_KEEPALIVE zipfile_to_fs(const char *file, const char* filter) {
    struct zip *zipArchive;
    struct zip_file *zipFile;
    struct zip_stat zipStat;
    char buf[ZIPTOFS_BUFFER_SIZE];
    int err;
    int i, len;
    int fd;
    long long sum;

    int32_t count;
    int32_t extracted = 0;
    int32_t filterLen = filter ? strlen(filter) : 0;

    int openFlags = O_RDWR | O_TRUNC | O_CREAT;

#ifdef __MINGW32__
    openFlags = openFlags | O_BINARY;
#endif

    if ((zipArchive = zip_open(file, 0, &err)) == 0) {
        zip_error_to_str(buf, sizeof(buf), err, errno);
        fprintf(stderr, "zip_to_fs: can't open zip archive: %s\n", buf);
        return 1;
    }

    count = zip_get_num_entries(zipArchive, 0);
    for (i = 0; i < count; i++) {
        if (zip_stat_index(zipArchive, i, 0, &zipStat) == 0) {
            len = strlen(zipStat.name);
            if (zipStat.name[len - 1] == '/') {
                safe_create_dir(zipStat.name);
            } else if (!filter || (strncmp(filter, zipStat.name, filterLen) == 0)) {
                zipFile = zip_fopen_index(zipArchive, i, 0);
                if (!zipFile) {
                    fprintf(stderr, "zip_to_fs: %s\n", zip_strerror(zipArchive));
                    fprintf(stderr,
                            "zip_to_fs: Try to repack archive with default zip program, error: '%s'\n",
                            zip_strerror(zipArchive));
                    exit(100);
                }
                fd = open(zipStat.name, openFlags, 0644);
                if (fd < 0) {
                    ensure_parent_dir(zipStat.name);
                    fd = open(zipStat.name, openFlags, 0644);
                    if (fd < 0) {
                        fprintf(stderr, "zip_to_fs: unable to write file %s\n",
                            zipStat.name);
                        exit(101);
                    }
                }
                sum = 0;
                while (sum != zipStat.size) {
                    len = zip_fread(zipFile, buf, ZIPTOFS_BUFFER_SIZE);
                    if (len < 0) {
                        fprintf(stderr, "zip_to_fs: %s\n", zip_file_strerror(zipFile));
                        exit(102);
                    }
                    write(fd, buf, len);
                    sum += len;
                }
                close(fd);
                zip_fclose(zipFile);

                lastExtractedMTimeMs = getMTimeMs(zipStat.name);
            }
        } else {
            printf("File[%s] Line[%d]\n", __FILE__, __LINE__);
        }

        extracted += 1;
#ifdef EMSCRIPTEN
        emsc_progress(zipStat.name, extracted, count);
#else
        if (onProgress != 0) {
            onProgress(zipStat.name, extracted, count);
        }
#endif
    }
    if (zip_close(zipArchive) == -1) {
        fprintf(stderr, "zip_to_fs: can't close zip archive %s\n", zip_strerror(zipArchive));
        return 1;
    }

    return 0;
}

void EMSCRIPTEN_KEEPALIVE libzip_destroy() {
#ifdef EMSCRIPTEN
    emscripten_force_exit(0);
#endif
}

int EMSCRIPTEN_KEEPALIVE zipfile_add(const char* archive, const char* nameInArchive, const char* nameInFs) {
    int err;
    zip_t* zipArchive = zip_open(archive, 0, &err);
    if (!zipArchive) {
        fprintf(stderr, "zipfile_add: can't open archive file: '%s', errp: %d\n", archive, err);
        return -1;
    }

    zip_source_t *source = zip_source_file(zipArchive, nameInFs, 0, 0);
    if (source == 0) {
        fprintf(stderr, "zipfile_add: can't create file %s, cause %s\n", nameInFs,
            zip_strerror(zipArchive));
        zip_close(zipArchive);
        return -1;
    }

    int index = zip_file_add(zipArchive, nameInArchive, source, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8);
    if (index == -1) {
        zip_source_free(source);
        fprintf(stderr, "zipfile_add: can't create file %s, cause %s\n", nameInFs,
                zip_strerror(zipArchive));
        zip_close(zipArchive);
        return -1;
    } else {
        if (zip_set_file_compression(zipArchive, index, ZIP_CM_STORE, 0) == -1) {
            fprintf(stderr, "zipfile_add: can't set compression level for %s, cause %s\n", nameInFs,
                    zip_strerror(zipArchive));
            return -1;
        }
    }
    
    zip_close(zipArchive);
    return 0;
}
