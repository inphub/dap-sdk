cmake_minimum_required(VERSION 3.10)
project (dap_http_server C)

include_directories(http_client/include)

file(GLOB HTTP_SERVER_SRCS FILES *.c http_client/*.c)
file(GLOB HTTP_SERVER_HDRS FILES include/*.h http_client/include/*.h)

if(WIN32)
    add_definitions ("-DUNDEBUG")
    add_definitions ("-DNDEBUG")
    add_definitions ("-DWIN32")
    add_definitions ("-D_WINDOWS")
    add_definitions ("-D__WINDOWS__")
    add_definitions ("-D_CRT_SECURE_NO_WARNINGS")
endif()

include_directories(${PROJECT_NAME} include/)
add_library(${PROJECT_NAME} STATIC ${HTTP_SERVER_SRCS} ${HTTP_SERVER_HDRS})
target_include_directories(${PROJECT_NAME} INTERFACE . http_client/include/)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../../3rdparty/uthash/src)

if(DARWIN)
    target_include_directories(${PROJECT_NAME} PRIVATE /usr/local/include)
endif()

if(WIN32)
  target_link_libraries(dap_http_server dap_core dap_crypto dap_io magic regex tre intl iconv
  kernel32
  user32
  shell32
  winmm
  gdi32
  advapi32
  ole32
  version
  imm32
  oleaut32
  ws2_32
  ntdll
  psapi
  shlwapi
  bcrypt
  crypt32
  secur32
)
endif()

if(UNIX)
  target_link_libraries(${PROJECT_NAME} dap_core dap_io)
  if(DAP_SDK_SUPPORT_LIBMAGIC)
        add_definitions("-DDAP_SDK_SUPPORT_LIBMAGIC")
        target_link_libraries(${PROJECT_NAME} magic json-c)
  endif()
  if(DAP_SDK_SUPPORT_JSON_C)
        add_definitions("-DDAP_SDK_SUPPORT_JSON_C")
        target_link_libraries(${PROJECT_NAME} json-c)
  endif()
  target_include_directories(${PROJECT_NAME} PUBLIC ${IPUTILS_INCLUDE_DIRS})
endif()

