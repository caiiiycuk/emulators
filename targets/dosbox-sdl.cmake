if (${EMSCRIPTEN})
else ()
    find_package(SDL REQUIRED)
    find_package(SDL_net REQUIRED)
endif ()

set(SOURCES_SDL_CXX03
        "${NATIVE_DIR}/dosbox/src/gui/sdlmain.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/mixer.cpp"
        "${NATIVE_DIR}/dosbox/src/gui/sdl_mapper.cpp"
        "${NATIVE_DIR}/dosbox/src/ints/mouse.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/ipxserver.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/ipx.cpp"
        #"${NATIVE_DIR}/jsdos-cpp/gui/sdlmain.cpp"
        )
set_source_files_properties(${SOURCES_SDL_CXX03} PROPERTIES COMPILE_FLAGS "${CORE_FLAGS} -std=c++03 -Wno-switch")
set(SOURCES_SERVER_SDL ${SOURCES_SERVER_CORE} ${SOURCES_SDL_CXX03})

add_executable(dosbox-sdl ${SOURCES_SERVER_SDL})
target_include_directories(dosbox-sdl PUBLIC "${SDL_INCLUDE_DIR}/..")
target_link_libraries(dosbox-sdl libdosbox-core)


if (${EMSCRIPTEN})
    set_target_properties(dosbox-sdl PROPERTIES SUFFIX .html)
    target_link_options(dosbox-sdl PUBLIC
            ${EM_LINK_OPTIONS}
            "-sMODULARIZE=0"
            "-sINVOKE_RUN=1"
            "--profiling-funcs"
            "-sASYNCIFY=1"
            "-sASYNCIFY_WHITELIST=@${TARGETS_DIR}/dosbox-asyncify.txt"
            "-sERROR_ON_UNDEFINED_SYMBOLS=0"
            )
elseif (MINGW)
    target_link_libraries(dosbox-sdl libsdl libsdl_net z ws2_32 mingw32 winmm)
elseif (APPLE)
    target_link_libraries(dosbox-sdl
            ${SDL_LIBRARIES}
            ${SDL_NET_LIBRARIES}
            "-framework CoreAudio"
            "-framework AudioToolbox"
            "-framework ForceFeedback"
            "-framework CoreVideo"
            "-framework Cocoa"
            "-framework Carbon"
            "-framework IOKit"
            "-weak_framework QuartzCore"
            "-weak_framework Metal"
            "-framework OpenGL"
            "-framework Carbon"
            "-framework CoreFoundation"
            "-framework CoreMIDI"
            "-framework AudioUnit"
            "-framework AudioToolbox"
            "-framework ApplicationServices"
            "-framework AppKit"
            "-framework IOKit"
            z)
else ()
    target_link_libraries(dosbox-sdl
            X11 z ncurses dl GL pthread asound
            SDL SDL_mixer SDL2_net
            )
endif ()

if (X86_64)
    target_compile_definitions(dosbox-sdl PUBLIC -DX86_64)
elseif (X86)
    target_compile_definitions(dosbox-sdl PUBLIC -DX86)
else ()
    set_target_properties(dosbox-sdl PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
endif ()
