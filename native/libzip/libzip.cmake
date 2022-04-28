if (${EMSCRIPTEN})
    set(LIBZIP_LINK_FLAGS "-s TOTAL_MEMORY=16777216")
endif ()

if (MINGW)
    add_definitions(-DZIP_STATIC)
endif()

SET(SOURCES_LIBZIP
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_add_entry.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_new.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_dir_add.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_set_default_password.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_extra_field_api.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_set_file_comment.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_strerror.c"
        # "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_random_uwp.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_layered.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_set_name.c"
        # "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_random_win32.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_file_rename.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_file_set_external_attributes.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_file_replace.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_err_str.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_algorithm_deflate.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_begin_write.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_error_get.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_get_num_files.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_hash.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_file_add.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_fseek.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_zip.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_extra_field.c"
        # "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_file_set_encryption.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_zip_new.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_random_unix.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_get_compression_flags.c"
        # "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_winzip_aes_encode.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_entry.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_set_file_compression.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_unchange.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_rename.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_fopen_index_encrypted.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_error_get_sys_type.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_free.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_close.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_file_get_external_attributes.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_libzip_version.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_commit_write.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_utf-8.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_delete.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_write.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_function.c"
        # "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_winzip_aes.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_unchange_archive.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_fdopen.c"
        # "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_crypto_openssl.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/mkstemp.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_file_error_get.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_file_set_comment.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_file_get_offset.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_is_deleted.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_get_file_comment.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_memdup.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_set_archive_comment.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_discard.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_get_archive_flag.c"
        # "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_crypto_commoncrypto.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_file_error_clear.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_io_util.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_rollback_write.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_error_to_str.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_seek_write.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_begin_write_cloning.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_error_clear.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_fclose.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_crc.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_ftell.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_filep.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_seek.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_get_name.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_file_strerror.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_fopen_index.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_pkware.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_tell_write.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_call.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_buffer.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_add_dir.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_set_archive_flag.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_filerange_crc.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_compress.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_get_num_entries.c"
        # "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_win32a.c"
        # "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_fopen_encrypted.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_add.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_buffer.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_stat.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_file_get_comment.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_error.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_name_locate.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_unchange_all.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_fread.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_file_set_mtime.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_error.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_replace.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_fopen.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_close.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_get_encryption_implementation.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_progress.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_remove.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_stat.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_supports.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_string.c"
        # "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_winzip_aes_decode.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_unchange_data.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_get_archive_comment.c"
        # "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_crypto_gnutls.c"
        # "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_algorithm_bzip2.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_stat_index.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_dirent.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_open.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_read.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_error_strerror.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_open.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_window.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_stat_init.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_tell.c"

        "${CMAKE_CURRENT_LIST_DIR}/jsdos-libzip.c"
        )

include_directories(
        "${CMAKE_CURRENT_LIST_DIR}/include/"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/"
)

if (MINGW)
    list(APPEND SOURCES_LIBZIP
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_win32utf8.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_win32w.c"
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_win32handle.c"
    )
else()
    list(APPEND SOURCES_LIBZIP
        "${CMAKE_CURRENT_LIST_DIR}/libzip-1.5/lib/zip_source_file.c"
    )
endif()

if (${EMSCRIPTEN})
    add_executable(wlibzip ${SOURCES_LIBZIP} "${CMAKE_CURRENT_LIST_DIR}/jsdos-libzip-js.c")
    set_target_properties(wlibzip PROPERTIES SUFFIX .js)
    set_target_properties(wlibzip PROPERTIES LINK_FLAGS "${LIBZIP_LINK_FLAGS} -s WASM=1 -s EXPORT_NAME='WLIBZIP'")
elseif (${BUILD_NATIVE_LIBZIP})
    add_executable(libzip ${SOURCES_LIBZIP} "${CMAKE_CURRENT_LIST_DIR}/jsdos-libzip-js.c")
    target_link_libraries(libzip z)
endif ()
