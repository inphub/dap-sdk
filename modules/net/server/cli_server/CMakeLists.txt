cmake_minimum_required(VERSION 3.10)
project (dap_cli_server C)

file(GLOB DAP_CLI_SRV_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

file(GLOB DAP_CLI_SRV_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

add_library(${PROJECT_NAME} STATIC ${DAP_CLI_SRV_SRCS} ${DAP_CLI_SRV_HEADERS} )

target_link_libraries(${PROJECT_NAME} dap_core dap_io m)
target_include_directories(${PROJECT_NAME} PUBLIC include/ )

INSTALL(FILES ${DAP_CLI_SRV_HEADERS} DESTINATION ${INCDIR})
