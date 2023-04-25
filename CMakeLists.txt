cmake_minimum_required(VERSION 3.10)
project (dap_sdk)
set(DAP_SDK_NATIVE_VERSION "3.0-0")
set(CMAKE_C_STANDARD 11)

file(GLOB DAP_SDK_SRCS
      *.c
)
file(GLOB DAP_SDK_HEADERS
      include/*.h
)

set(DAP_SDK_SRC_EXT "")
set(DAP_SDK_HEADERS_EXT "")
include_directories(include)

option(DAP_SDK_BUILD_STATIC      "Static project build scheme" OFF)
option(DAP_SDK_SINGLE_BUILD      "Single project build scheme" ON)
option(DAP_SDK_DO_TESTS          "Build and run tests" OFF)

option(DAP_SDK_MODULE_CORE       "DAP SDK Core        module" ON)
option(DAP_SDK_MODULE_CRYPTO     "DAP SDK Crypto      module" ON)
option(DAP_SDK_MODULE_IO         "DAP SDK IO          module" ON)
option(DAP_SDK_MODULE_APP_CLI    "DAP SDK App Cli     module" ON)
option(DAP_SDK_MODULE_NET_CORE   "DAP SDK Net Core    module" OFF)
option(DAP_SDK_MODULE_NET_CLIENT "DAP SDK Net Client  module" OFF)
option(DAP_SDK_MODULE_NET_SERVER "DAP SDK Net Server  module" OFF)
option(DAP_SDK_MODULE_PLUGINS    "DAP SDK Plugins     module" OFF)

option(DAP_SDK_SUPPORT_LIBMAGIC  "DAP SDK libmagic support" OFF)
option(DAP_SDK_SUPPORT_SSL       "DAP SDK SSL      support" OFF)
option(DAP_SDK_SUPPORT_JSON_C    "DAP SDK JSON-C   support" ON)

set(BINDIR ${CMAKE_INSTALL_PREFIX}/bin)
set(INCDIR ${CMAKE_INSTALL_PREFIX}/include)
set(CONFIGDIR ${CMAKE_INSTALL_PREFIX}/etc)
set(SBINDIR ${CMAKE_INSTALL_PREFIX}/sbin)
set(LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
set(RLIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
set(SHAREDIR ${CMAKE_INSTALL_PREFIX}/share)
set(CMAKEDIR ${CMAKE_INSTALL_PREFIX}/share/cmake)


if(UNIX)
        set(DAP_SDK_SRC_EXT "${DAP_SDK_SRC_EXT};${PROJECT_SOURCE_DIR}/os/unix/sig_unix_handler.c")
        set(DAP_SDK_HEADERS_EXT "${DAP_SDK_HEADERS_EXT};${PROJECT_SOURCE_DIR}/os/unix/sig_unix_handler.h")
        include_directories(os/unix)
endif()

if(DAP_SDK_SINGLE_BUILD)
        include(share/cmake/OS_Detection.cmake)
        # Перечисление модулей DAP SDK которые нам нужны
        add_definitions("-DDAP_VERSION=\"${DAP_SDK_NATIVE_VERSION}\"")
endif()

if(DAP_SDK_BUILD_STATIC OR BUILD_STATIC)
        message("Build static library ${PROJECT_NAME}")
        add_library(${PROJECT_NAME} STATIC ${DAP_SDK_SRCS} ${DAP_SDK_SRC_EXT} ${DAP_SDK_HEADERS} ${DAP_SDK_HEADERS_EXT})
else()
        message("Build shared library ${PROJECT_NAME}")
        add_library(${PROJECT_NAME} SHARED ${DAP_SDK_SRCS} ${DAP_SDK_SRC_EXT} ${DAP_SDK_HEADERS} ${DAP_SDK_HEADERS_EXT})
endif()


# We switch off SSL until its not switched on back
if(DAP_SDK_SUPPORT_SSL)
    add_definitions("-DDAP_NET_CLIENT_SSL")
else()
    add_definitions("-DDAP_NET_CLIENT_NO_SSL")
endif()

set(DAP_SDK_PUBLIC_HEADERS "")


# Core, add it always
if(DAP_SDK_MODULE_CORE)
    add_subdirectory(modules/core)
#    target_link_libraries(${PROJECT_NAME} dap_core)
endif()

if (DAP_SDK_MODULE_CRYPTO)
    # Cryptography
    add_subdirectory(modules/crypto)
#    target_link_libraries(${PROJECT_NAME} dap_core dap_crypto)
endif()

if (DAP_SDK_MODULE_APP_CLI)
#    add_subdirectory(modules/net/app-cli)
#    add_subdirectory(modules/net/server/cli_server )
#    target_link_libraries(${PROJECT_NAME} dap_core dap_app_cli dap_cli_server)
endif()

if (DAP_SDK_MODULE_PLUGINS)
    add_subdirectory(plugins)
#    target_link_libraries(${PROJECT_NAME} dap_plugins)
endif()

# I/O subsystem
if (DAP_SDK_MODULE_IO)
    add_subdirectory(modules/io)
#    target_link_libraries(${PROJECT_NAME} dap_core dap_io)
endif()

# Networking core
if (DAP_SDK_MODULE_NET_CLIENT OR DAP_SDK_MODULE_NET_SERVER OR DAP_SDK_MODULE_NET_CORE)
    add_subdirectory(modules/net/stream)
#    target_link_libraries(${PROJECT_NAME} dap_core libdap-stream)
endif()

#Networking client
if (DAP_SDK_MODULE_NET_CLIENT)
#    target_link_libraries(${PROJECT_NAME} dap_core dap_client)
endif()

# Networking server
if (DAP_SDK_MODULE_NET_SERVER)
#    set(DAP_SDK_MODULES_LIBS "${DAP_SDK_MODULES_LIBS} libdap-server")
endif()
#add_subdirectory(3rdparty/json-c)
add_subdirectory(modules/net/app-cli)
#add_subdirectory(modules/net/server/)
add_subdirectory(modules/net/server/cli_server)
#add_subdirectory(modules/net/stream)
#add_subdirectory(modules/net/client)
target_link_libraries(${PROJECT_NAME} dap_core dap_crypto dap_io dap_cli_server dap_app_cli)# libdap-stream libdap-server dap_http_client dap_client dap_app_cli)
#add_subdirectory(modules/net/server/cli_server )

target_include_directories(${PROJECT_NAME} INTERFACE . include/)

# Build and run tests
if(DAP_SDK_DO_TESTS)
add_subdirectory(tests)
endif()

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${LIBDIR})
INSTALL(FILES ${DAP_SDK_HEADERS} DESTINATION ${INCDIR})

# Cmake install
file(GLOB CMAKE_FILES cmake/*)
INSTALL(FILES ${CMAKE_FILES} DESTINATION ${CMAKEDIR} )


# 3rd party headers
file(GLOB 3RDPARTY_HEADERS_UTHASH 3rdparty/uthash/src/*.h)
INSTALL(FILES ${3RDPARTY_HEADERS_UTHASH} DESTINATION ${INCDIR})

