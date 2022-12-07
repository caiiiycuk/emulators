set(DEFENITIONS_CORE
        -DHAVE_CONFIG_H
        -DGET_X86_FUNCTIONS
        -DJSDOS
        -DC_IPX
        -DWITHOUT_SDL
        )

set(INCLUDE_DIRECTORIES_CORE
        "${NATIVE_DIR}/config"
        "${NATIVE_DIR}/sdl2net"
        "${NATIVE_DIR}/jsdos/include"
        "${NATIVE_DIR}/dosbox/include"
        "${NATIVE_DIR}/dosbox"
        )

set(SOURCES_CORE_CXX11
        "${NATIVE_DIR}/jsdos/jsdos-log.cpp"
        "${NATIVE_DIR}/jsdos/jsdos-asyncify.cpp"
        "${NATIVE_DIR}/jsdos/jsdos-timer.cpp"
        "${NATIVE_DIR}/jsdos/jsdos-support.cpp"
        "${NATIVE_DIR}/jsdos/jsdos-events.cpp"
        "${NATIVE_DIR}/dosbox/src/dosbox.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/dos_files.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/cdrom_image.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/pic.cpp"
        )


set(SOURCES_CORE_CXX03
        "${NATIVE_DIR}/dosbox/src/cpu/core_simple.cpp"
        "${NATIVE_DIR}/dosbox/src/cpu/paging.cpp"
        "${NATIVE_DIR}/dosbox/src/cpu/core_prefetch.cpp"
        "${NATIVE_DIR}/dosbox/src/cpu/core_dyn_x86.cpp"
        "${NATIVE_DIR}/dosbox/src/cpu/core_full.cpp"
        "${NATIVE_DIR}/dosbox/src/cpu/core_dynrec.cpp"
        "${NATIVE_DIR}/dosbox/src/cpu/core_normal.cpp"
        "${NATIVE_DIR}/dosbox/src/cpu/cpu.cpp"
        "${NATIVE_DIR}/dosbox/src/cpu/callback.cpp"
        "${NATIVE_DIR}/dosbox/src/cpu/flags.cpp"
        "${NATIVE_DIR}/dosbox/src/cpu/modrm.cpp"
        "${NATIVE_DIR}/dosbox/src/fpu/fpu.cpp"
        "${NATIVE_DIR}/dosbox/src/ints/int10_pal.cpp"
        "${NATIVE_DIR}/dosbox/src/ints/ems.cpp"
        "${NATIVE_DIR}/dosbox/src/ints/xms.cpp"
        "${NATIVE_DIR}/dosbox/src/ints/int10_put_pixel.cpp"
        "${NATIVE_DIR}/dosbox/src/ints/bios_disk.cpp"
        "${NATIVE_DIR}/dosbox/src/ints/bios.cpp"
        "${NATIVE_DIR}/dosbox/src/ints/int10_misc.cpp"
        "${NATIVE_DIR}/dosbox/src/ints/int10_char.cpp"
        "${NATIVE_DIR}/dosbox/src/ints/int10.cpp"
        "${NATIVE_DIR}/dosbox/src/ints/int10_video_state.cpp"
        "${NATIVE_DIR}/dosbox/src/ints/int10_memory.cpp"
        "${NATIVE_DIR}/dosbox/src/ints/int10_vptable.cpp"
        "${NATIVE_DIR}/dosbox/src/ints/bios_keyboard.cpp"
        "${NATIVE_DIR}/dosbox/src/ints/int10_vesa.cpp"
        "${NATIVE_DIR}/dosbox/src/ints/int10_modes.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/drive_local.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/drive_cache.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/dos_keyboard_layout.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/cdrom.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/cdrom_ioctl_linux.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/cdrom_ioctl_win32.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/cdrom_aspi_win32.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/cdrom_ioctl_os2.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/drive_overlay.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/dos_tables.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/dos_devices.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/dos_mscdex.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/dos_execute.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/dos_programs.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/dos.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/drive_virtual.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/drives.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/dos_ioctl.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/dos_memory.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/dos_classes.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/dos_misc.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/drive_fat.cpp"
        "${NATIVE_DIR}/dosbox/src/dos/drive_iso.cpp"
        "${NATIVE_DIR}/dosbox/src/shell/shell.cpp"
        "${NATIVE_DIR}/dosbox/src/shell/shell_cmds.cpp"
        "${NATIVE_DIR}/dosbox/src/shell/shell_batch.cpp"
        "${NATIVE_DIR}/dosbox/src/shell/shell_misc.cpp"
        "${NATIVE_DIR}/dosbox/src/gui/midi.cpp"
        "${NATIVE_DIR}/dosbox/src/gui/sdl_gui.cpp"
        "${NATIVE_DIR}/dosbox/src/gui/render_scalers.cpp"
        "${NATIVE_DIR}/dosbox/src/gui/render.cpp"
        "${NATIVE_DIR}/dosbox/src/debug/debug.cpp"
        "${NATIVE_DIR}/dosbox/src/debug/debug_win32.cpp"
        "${NATIVE_DIR}/dosbox/src/debug/debug_disasm.cpp"
        "${NATIVE_DIR}/dosbox/src/debug/debug_gui.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/pcspeaker.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/sblaster.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/gus.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/vga_attr.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/vga_paradise.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/dma.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/vga_xga.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/timer.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/vga_dac.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/vga_s3.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/cmos.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/vga_memory.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/dbopl.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/vga_crtc.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/vga.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/hardware.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/vga_draw.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/adlib.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/iohandler.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/joystick.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/vga_seq.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/pci_bus.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/mpu401.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/keyboard.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/vga_misc.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/tandy_sound.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/vga_tseng.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/vga_gfx.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/vga_other.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/gameblaster.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/disney.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/memory.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/serialport/directserial.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/serialport/serialdummy.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/serialport/nullmodem.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/serialport/serialport.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/serialport/libserial.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/serialport/softmodem.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/serialport/misc_util.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/mame/fmopl.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/mame/saa1099.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/mame/sn76496.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/mame/ymdeltat.cpp"
        "${NATIVE_DIR}/dosbox/src/hardware/mame/ymf262.cpp"
        "${NATIVE_DIR}/dosbox/src/misc/programs.cpp"
        "${NATIVE_DIR}/dosbox/src/misc/setup.cpp"
        "${NATIVE_DIR}/dosbox/src/misc/cross.cpp"
        "${NATIVE_DIR}/dosbox/src/misc/support.cpp"
        "${NATIVE_DIR}/dosbox/src/misc/messages.cpp"
        #	"${NATIVE_DIR}/dosbox/src/libs/gui_tk/gui_tk.cpp"
        #	"${NATIVE_DIR}/dosbox/src/hardware/opl.cpp"
        #	"${NATIVE_DIR}/dosbox/src/libs/zmbv/drvproc.cpp"
        #	"${NATIVE_DIR}/dosbox/src/libs/zmbv/zmbv.cpp"
        #	"${NATIVE_DIR}/dosbox/src/libs/zmbv/zmbv_vfw.cpp"
        )

set(SOURCES_JSDOS_CXX11
        "${NATIVE_DIR}/jsdos/dosbox/jsdos-main.cpp"
        "${NATIVE_DIR}/jsdos/dosbox/jsdos-mixer.cpp"
        "${NATIVE_DIR}/jsdos/jsdos-mapper-noop.cpp"
        "${NATIVE_DIR}/jsdos/jsdos-noop.cpp"
        "${NATIVE_DIR}/jsdos/jsdos-mouse.cpp"
        "${NATIVE_DIR}/jsdos/jsdos-ipx.cpp"
        )

set(SOURCES_JSDOS_NET_C
        ${NATIVE_DIR}/sdl2net/SDLnet.c
        ${NATIVE_DIR}/sdl2net/SDLnetTCP.c
        ${NATIVE_DIR}/sdl2net/SDLnetselect.c
        )

set_source_files_properties(${SOURCES_CORE_CXX03} PROPERTIES COMPILE_FLAGS "${CORE_FLAGS} -std=c++03 -Wno-switch")
set_source_files_properties(${SOURCES_CORE_CXX11} PROPERTIES COMPILE_FLAGS "${CORE_FLAGS} -std=c++11")
set_source_files_properties(${SOURCES_JSDOS_CXX11} PROPERTIES COMPILE_FLAGS "${CORE_FLAGS} -std=c++11")

add_library(libdosbox-core OBJECT ${SOURCES_CORE_CXX11} ${SOURCES_CORE_CXX03})
target_compile_definitions(libdosbox-core PUBLIC ${DEFENITIONS_CORE})
target_include_directories(libdosbox-core PUBLIC ${INCLUDE_DIRECTORIES_CORE})

add_library(libdosbox OBJECT ${SOURCES_JSDOS_CXX11} ${SOURCES_JSDOS_NET_C})
target_link_libraries(libdosbox libdosbox-core)

if (MINGW)
    target_compile_definitions(dosbox PUBLIC -DWIN32)
endif ()

if (${EMSCRIPTEN})
    add_executable(wdosbox
        "${SRC_DIR}/dos/dosbox/cpp/worker-protocol.cpp"
        "${SRC_DIR}/dos/dosbox/cpp/direct-debugger.cpp")
    target_link_libraries(wdosbox libdosbox libdosbox-core libzip)
    set_target_properties(wdosbox PROPERTIES SUFFIX .js)
    target_link_options(wdosbox PUBLIC
            "${EM_LINK_OPTIONS}"
            "-sUSE_ZLIB=1"
            "-sWASM=1"
            "-sASYNCIFY=1"
            "-sASYNCIFY_IMPORTS=['syncSleep']"
            "-sASYNCIFY_WHITELIST=@${TARGETS_DIR}/dosbox-asyncify.txt"
            "-sEXPORT_NAME='WDOSBOX'")
else ()
    target_include_directories(libdosbox-core PUBLIC "${NATIVE_DIR}/jsdos/linux")
endif ()


if (X86_64)
    target_compile_definitions(libdosbox-core PUBLIC -DX86_64)
elseif (X86)
    target_compile_definitions(libdosbox-core PUBLIC -DX86)
else ()
    set_target_properties(libdosbox-core PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
endif ()

