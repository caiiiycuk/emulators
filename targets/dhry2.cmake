set(DHRY2_DIR "${NATIVE_DIR}/dhry2")
set(SOURCES_DHRY2_CXX11
        "${DHRY2_DIR}/dhry2.cpp"
        )

set(SOURCES_DHRY2_CXX03
        )

set_source_files_properties(${SOURCES_DHRY2_CXX11} PROPERTIES COMPILE_FLAGS "${OPT_FLAGS} -std=c++11")
set_source_files_properties(${SOURCES_DHRY2_CXX03} PROPERTIES COMPILE_FLAGS "${OPT_FLAGS} -std=c++03")

add_executable(dosbox-dhry2 ${SOURCES_DHRY2_CXX11} ${SOURCES_DHRY2_CXX03})
target_link_libraries(dosbox-dhry2 libdosbox libdosbox-core)
add_custom_command(TARGET dosbox-dhry2
        PRE_BUILD
        COMMAND cp "${DHRY2_DIR}/bin/dhry_2.exe" /tmp/dhry_2.exe && xxd -i /tmp/dhry_2.exe > "${DHRY2_DIR}/dhry_2_exe.h"
        COMMAND cp "${DHRY2_DIR}/bin/DOS4GW.EXE" /tmp/dos4gw.exe && xxd -i /tmp/dos4gw.exe > "${DHRY2_DIR}/dos4gw_exe.h"
        COMMAND cp "${DHRY2_DIR}/bin/dosbox.conf" /tmp/dosbox.conf && xxd -i /tmp/dosbox.conf > "${DHRY2_DIR}/dosbox_conf.h"
        COMMAND cp "${DHRY2_DIR}/bin/dosbox-dhry2.html" "${CMAKE_BINARY_DIR}/dosbox-dhry2.html"
        COMMAND cp "${DHRY2_DIR}/bin/dosbox-dhry2-node.js" "${CMAKE_BINARY_DIR}/dosbox-dhry2-node.js"
        )

if (${EMSCRIPTEN})
    set_target_properties(dosbox-dhry2 PROPERTIES SUFFIX .js)
    target_link_options(dosbox-dhry2 PUBLIC
            "${EM_LINK_OPTIONS}"
            "-sWASM=1"
            "-sASYNCIFY=1"
            "-sASYNCIFY_IMPORTS=['syncSleep']"
            "-sASYNCIFY_WHITELIST=@${TARGETS_DIR}/dosbox-asyncify.txt"
            "-sEXPORT_NAME='WDHRY2'")
endif ()
