cmake_minimum_required(VERSION 3.10)
project (dap_core)

add_definitions("-D_GNU_SOURCE") 

file(GLOB CORE_SRCS
  src/*.c
  src/etc/*.c
  src/rpmalloc/*.c
)
file(GLOB CORE_HEADERS
  include/*.h
  include/unix/*.h
  include/unix/linux/*.h
)

add_library(${PROJECT_NAME} STATIC ${CORE_SRCS} ${CORE_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/uthash/src)
#if (WIN32)
#    target_include_directories(${PROJECT_NAME} PRIVATE ../../3rdparty/wepoll)
#endif()

#This paths will be used by project-dependent project libraries
target_include_directories(${PROJECT_NAME} INTERFACE include/ src/rpmalloc/ src/FIPS202/ src/rand)

if(WIN32)
  include_directories(include/)
endif()

if(UNIX)
    add_subdirectory(src/unix)
    set(CORE_LIBRARIES dap_core_unix)
    if(DARWIN)
        add_subdirectory(src/darwin)
        set(CORE_LIBRARIES ${CORE_LIBRARIES} dap_core_darwin)
    else()
        EXECUTE_PROCESS( COMMAND ${CMAKE_C_COMPILER} --version COMMAND grep gcc COMMAND awk "{print $4;}" OUTPUT_VARIABLE C_COMPILER_VERSION)
        message("C compiler version ${C_COMPILER_VERSION}")
        if (DAP_SDK_LINK_ATOMIC)
            set(CORE_LIBRARIES ${CORE_LIBRARIES} rt atomic)
            message("--link with libatomic")
        else()
            set(CORE_LIBRARIES ${CORE_LIBRARIES} rt)
            message("--link without libatomic")
        endif()
    endif()
    if(ANDROID)
        add_subdirectory(src/android)
        set(CORE_LIBRARIES ${CORE_LIBRARIES} dap_core_android)
    endif()
endif()

if (WIN32)
    add_subdirectory(src/win32)
    set(CORE_LIBRARIES dap_core_win32)
endif()

if (BUILD_WITH_ZIP)
    set(CORE_LIBRARIES ${CORE_LIBRARIES} zip)
    add_definitions ("-DDAP_BUILD_WITH_ZIP")
endif()

target_link_libraries(${PROJECT_NAME} ${CORE_LIBRARIES})

if(BUILD_DAP_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

INSTALL(FILES ${CORE_HEADERS} DESTINATION ${INCDIR})
