include_directories(
        "${NATIVE_DIR}/sokol-lib"
)

set(SOURCES_SOKOL_CLIENT
        "${NATIVE_DIR}/sokol/protocol-sokol-client.cpp"
        )

set(SOURCES_SOKOL
        "${NATIVE_DIR}/sokol/protocol-sokol.cpp"
        )

set_source_files_properties(${SOURCES_SOKOL} PROPERTIES COMPILE_FLAGS "-std=c++14")

if (APPLE)
    set_source_files_properties(${SOURCES_SOKOL_CLIENT} PROPERTIES COMPILE_FLAGS "-ObjC++ -std=c++11")
else ()
    set_source_files_properties(${SOURCES_SOKOL_CLIENT} PROPERTIES COMPILE_FLAGS "-std=c++11")
endif ()

if (${EMSCRIPTEN})
else ()
    add_executable(dosbox-sokol ${SOURCES_SOKOL} ${SOURCES_SOKOL_CLIENT})
    target_link_libraries(dosbox-sokol libdosbox libdosbox-core)

    if (APPLE)
        target_link_libraries(dosbox-sokol
                "-framework Cocoa"
                "-framework QuartzCore"
                "-framework OpenGL"
                "-framework CoreAudio"
                "-framework AudioToolbox"
                z)
    elseif (MINGW)
        target_link_libraries(dosbox-sokol ws2_32 z winmm)
    else ()
        target_link_libraries(dosbox-sokol X11 Xcursor Xi z ncurses dl GL pthread asound)
    endif ()

    if (X86_64)
        target_compile_definitions(dosbox-sokol PUBLIC -DX86_64)
    elseif (X86)
        target_compile_definitions(dosbox-sokol PUBLIC -DX86)
    else ()
        set_target_properties(dosbox-sokol PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
    endif ()
endif ()
