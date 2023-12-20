project(js-dos)
cmake_minimum_required(VERSION 3.21)

set(X86 false)
if (${EMSCRIPTEN})
	set(X86_64 false)
else ()
	set(X86_64 true)
endif ()

set(BUILD_NATIVE_LIBZIP true)

set(SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src")
set(NATIVE_DIR "${CMAKE_CURRENT_LIST_DIR}/native")
set(TARGETS_DIR "${CMAKE_CURRENT_LIST_DIR}/targets")

# add_compile_options(-fno-exceptions)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w -Werror=return-type -fsigned-char")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -Werror=return-type -fsigned-char") 

if ((CMAKE_BUILD_TYPE STREQUAL "Debug") OR (CMAKE_BUILD_TYPE STREQUAL ""))
	message(STATUS "DEBUG BUILD")
else ()
	message(STATUS "RELEASE BUILD")
	add_compile_options(-Oz)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Oz -flto=thin")
endif()

if(${EMSCRIPTEN})
	add_definitions(-DEMSCRIPTEN)

	set(EM_LINK_OPTIONS
		"-Wl,-u,htons" # https://github.com/emscripten-core/emscripten/issues/16836
		"-Wl,-u,ntohs" # https://github.com/emscripten-core/emscripten/issues/16836
		"--emit-symbol-map"
		"-sTOTAL_STACK=1MB"
		"-sMALLOC=emmalloc"
		"-sASSERTIONS=0"
		"-sSAFE_HEAP=0"
		"-sTOTAL_MEMORY=67108864"
		"-sALLOW_MEMORY_GROWTH=1"
		"-sEXIT_RUNTIME=1"
		"-sINVOKE_RUN=0"
		"-sMODULARIZE=1"
		"-sFORCE_FILESYSTEM=1"
		"-sEXTRA_EXPORTED_RUNTIME_METHODS=['addRunDependency', 'removeRunDependency','FS', 'FS_createPath', 'FS_createPreloadedFile', \
			'FS_createDataFile', 'lengthBytesUTF8', 'stringToUTF8', 'UTF16ToString', 'UTF8ToString', 'callMain', 'err']"
		"-sEXPORTED_FUNCTIONS=['_main', '_malloc', '_free', '_abort']"
		"-sERROR_ON_UNDEFINED_SYMBOLS=1")
elseif(APPLE)
	add_definitions(-DEMSCRIPTEN_KEEPALIVE=)
	add_definitions(-DMACOSX)
	set(X86_64 true)
elseif(MINGW)
	add_definitions(-DEMSCRIPTEN_KEEPALIVE=)
	set(X86_64 true)
else()
	add_definitions(-DEMSCRIPTEN_KEEPALIVE=)
	set(CMAKE_C_COMPILER "/usr/bin/clang")
	set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
#	add_definitions(-DC_DEBUG=1 -DC_HEAVY_DEBUG=1)
endif()

include_directories(
		"${SRC_DIR}/protocol"
		"${NATIVE_DIR}/sockdrive/include"
)

# tier 1
include("${TARGETS_DIR}/zip.cmake")
include("${TARGETS_DIR}/dosbox.cmake")
include("${TARGETS_DIR}/dhry2.cmake")
include("${TARGETS_DIR}/sokol.cmake")

# tier 2
include("${TARGETS_DIR}/dosbox-sdl.cmake")
include("${TARGETS_DIR}/dosbox-x-sdl2.cmake")

add_custom_target(sokol-shaders
		COMMAND "${CMAKE_CURRENT_LIST_DIR}/native/sokol-tools-bin/bin/linux/sokol-shdc"
		--input "${CMAKE_CURRENT_LIST_DIR}/native/sokol/shaders.glsl"
		--output "${CMAKE_CURRENT_LIST_DIR}/native/sokol/shaders.h"
		--slang glsl330)