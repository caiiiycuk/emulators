if (${EMSCRIPTEN})
  set(WORKER_LINK_FLAGS "")
endif ()

set(SOURCES_WORKER_CXX11
  "${CMAKE_CURRENT_LIST_DIR}/cpp/worker-protocol.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/cpp/direct-debugger.cpp"
  )

set(SOURCES_WORKER_CXX03
  )

set_source_files_properties(${SOURCES_WORKER_CXX11} PROPERTIES COMPILE_FLAGS "${OPT_FLAGS} -std=c++11")
set_source_files_properties(${SOURCES_WORKER_CXX03} PROPERTIES COMPILE_FLAGS "${OPT_FLAGS} -std=c++03")

set(SOURCES_SERVER_WORKER
  ${SOURCES_SERVER_JSDOS}
  ${SOURCES_LIBZIP}
  ${SOURCES_WORKER_CXX11}
  ${SOURCES_WORKER_CXX03}
  )

if (${EMSCRIPTEN})
  add_library(worker-server OBJECT ${SOURCES_SERVER_WORKER})

  add_executable(wdosbox $<TARGET_OBJECTS:worker-server>)
  set_target_properties(wdosbox PROPERTIES SUFFIX .js)
  set_target_properties(wdosbox PROPERTIES LINK_FLAGS "${WORKER_LINK_FLAGS} -s WASM=1 -s ASYNCIFY=1 -s 'ASYNCIFY_IMPORTS=[\"syncSleep\"]' -s ASYNCIFY_WHITELIST=@${CMAKE_CURRENT_LIST_DIR}/../../../native/dosbox-jsdos/asyncify.txt -s EXPORT_NAME='WDOSBOX'")

  add_library(worker-server-shared OBJECT ${SOURCES_SERVER_WORKER})
  set_target_properties(worker-server-shared PROPERTIES COMPILE_OPTIONS "-sSHARED_MEMORY=1")

  add_executable(wdosbox.shared $<TARGET_OBJECTS:worker-server-shared>)
  set_target_properties(wdosbox.shared PROPERTIES SUFFIX .js)
  set_target_properties(wdosbox.shared PROPERTIES LINK_FLAGS "${WORKER_LINK_FLAGS} -sSHARED_MEMORY=1 -sUSE_PTHREADS=1 -s WASM=1 -s ASYNCIFY=1 -s 'ASYNCIFY_IMPORTS=[\"syncSleep\"]' -s ASYNCIFY_WHITELIST=@${CMAKE_CURRENT_LIST_DIR}/../../../native/dosbox-jsdos/asyncify.txt -s EXPORT_NAME='WDOSBOX'")
endif ()
