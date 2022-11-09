set(LIBZIP_DIR "${NATIVE_DIR}/libzip")

SET(SOURCES_LIBZIP
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_add_entry.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_new.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_dir_add.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_set_default_password.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_extra_field_api.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_set_file_comment.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_strerror.c"
        # "${LIBZIP_DIR}/libzip-1.5/lib/zip_random_uwp.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_layered.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_set_name.c"
        # "${LIBZIP_DIR}/libzip-1.5/lib/zip_random_win32.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_file_rename.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_file_set_external_attributes.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_file_replace.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_err_str.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_algorithm_deflate.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_begin_write.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_error_get.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_get_num_files.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_hash.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_file_add.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_fseek.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_zip.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_extra_field.c"
        # "${LIBZIP_DIR}/libzip-1.5/lib/zip_file_set_encryption.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_zip_new.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_random_unix.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_get_compression_flags.c"
        # "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_winzip_aes_encode.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_entry.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_set_file_compression.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_unchange.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_rename.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_fopen_index_encrypted.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_error_get_sys_type.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_free.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_close.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_file_get_external_attributes.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_libzip_version.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_commit_write.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_utf-8.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_delete.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_write.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_function.c"
        # "${LIBZIP_DIR}/libzip-1.5/lib/zip_winzip_aes.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_unchange_archive.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_fdopen.c"
        # "${LIBZIP_DIR}/libzip-1.5/lib/zip_crypto_openssl.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/mkstemp.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_file_error_get.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_file_set_comment.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_file_get_offset.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_is_deleted.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_get_file_comment.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_memdup.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_set_archive_comment.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_discard.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_get_archive_flag.c"
        # "${LIBZIP_DIR}/libzip-1.5/lib/zip_crypto_commoncrypto.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_file_error_clear.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_io_util.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_rollback_write.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_error_to_str.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_seek_write.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_begin_write_cloning.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_error_clear.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_fclose.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_crc.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_ftell.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_filep.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_seek.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_get_name.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_file_strerror.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_fopen_index.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_pkware.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_tell_write.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_call.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_buffer.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_add_dir.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_set_archive_flag.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_filerange_crc.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_compress.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_get_num_entries.c"
        # "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_win32a.c"
        # "${LIBZIP_DIR}/libzip-1.5/lib/zip_fopen_encrypted.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_add.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_buffer.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_stat.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_file_get_comment.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_error.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_name_locate.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_unchange_all.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_fread.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_file_set_mtime.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_error.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_replace.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_fopen.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_close.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_get_encryption_implementation.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_progress.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_remove.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_stat.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_supports.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_string.c"
        # "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_winzip_aes_decode.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_unchange_data.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_get_archive_comment.c"
        # "${LIBZIP_DIR}/libzip-1.5/lib/zip_crypto_gnutls.c"
        # "${LIBZIP_DIR}/libzip-1.5/lib/zip_algorithm_bzip2.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_stat_index.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_dirent.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_open.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_read.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_error_strerror.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_open.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_window.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_stat_init.c"
        "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_tell.c"
        "${LIBZIP_DIR}/jsdos-libzip.c"
        )

if (MINGW)
    list(APPEND SOURCES_LIBZIP
            "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_win32utf8.c"
            "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_win32w.c"
            "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_win32handle.c"
            )
else ()
    list(APPEND SOURCES_LIBZIP
            "${LIBZIP_DIR}/libzip-1.5/lib/zip_source_file.c"
            )
endif ()

add_library(libzip OBJECT ${SOURCES_LIBZIP})
target_include_directories(libzip PUBLIC
        "${LIBZIP_DIR}/include/"
        "${LIBZIP_DIR}/libzip-1.5/"
        "${LIBZIP_DIR}/libzip-1.5/lib/"
        )
if (MINGW)
    target_compile_definitions(libzip PUBLIC -DZIP_STATIC)
endif ()

if (${EMSCRIPTEN})
    add_executable(wlibzip "${LIBZIP_DIR}/jsdos-libzip-js.c")
    target_link_libraries(wlibzip libzip)
    set_target_properties(wlibzip PROPERTIES SUFFIX .js)
    target_link_options(wlibzip PUBLIC
            ${EM_LINK_OPTIONS}
            "-sTOTAL_MEMORY=16777216"
            "-sUSE_ZLIB=1"
            "-sWASM=1"
            "-sEXPORT_NAME='WLIBZIP'")

elseif (${BUILD_NATIVE_LIBZIP})
    add_executable(zip "${LIBZIP_DIR}/jsdos-libzip-js.c")
    target_link_libraries(zip libzip z)
endif ()
