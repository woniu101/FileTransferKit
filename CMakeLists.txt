cmake_minimum_required(VERSION 3.10)

project(FileTransferKit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(FILETRANSFERKIT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(FILETRANSFERKIT_BIN_DIR ${FILETRANSFERKIT_ROOT}/bin)
set(FILETRANSFERKIT_LIB_DIR ${FILETRANSFERKIT_ROOT}/lib)
set(FILETRANSFERKIT_QSFTP_ROOT ${FILETRANSFERKIT_ROOT}/3rdparty/qsftp)
set(FILETRANSFERKIT_QFTP_ROOT ${FILETRANSFERKIT_ROOT}/3rdparty/qftp)

find_package(Qt5 REQUIRED COMPONENTS Core Network Gui Widgets)
find_package(Threads REQUIRED)

file(GLOB QSSH_SOURCES
    ${FILETRANSFERKIT_QSFTP_ROOT}/src/*.cpp
)

list(APPEND QSSH_SOURCES
    ${FILETRANSFERKIT_QSFTP_ROOT}/src/sshkeycreationdialog.ui
    ${FILETRANSFERKIT_QSFTP_ROOT}/src/3rdparty/botan/botan.cpp
)

add_library(QSsh SHARED ${QSSH_SOURCES})

target_include_directories(QSsh
    PUBLIC
        ${FILETRANSFERKIT_QSFTP_ROOT}/include
    PRIVATE
        ${FILETRANSFERKIT_QSFTP_ROOT}/src
        ${FILETRANSFERKIT_QSFTP_ROOT}/src/3rdparty
)

target_compile_definitions(QSsh
    PRIVATE
        QSSH_LIBRARY
        BOTAN_DLL=
)

if(UNIX)
    target_compile_definitions(QSsh
        PRIVATE
            BOTAN_TARGET_OS_HAS_GETTIMEOFDAY
            BOTAN_HAS_ALLOC_MMAP
            BOTAN_HAS_ENTROPY_SRC_DEV_RANDOM
            BOTAN_HAS_ENTROPY_SRC_EGD
            BOTAN_HAS_ENTROPY_SRC_FTW
            BOTAN_HAS_ENTROPY_SRC_UNIX
            BOTAN_HAS_MUTEX_PTHREAD
            BOTAN_HAS_PIPE_UNIXFD_IO
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(QSsh
        PRIVATE
            BOTAN_TARGET_OS_IS_LINUX
            BOTAN_TARGET_OS_HAS_CLOCK_GETTIME
            BOTAN_TARGET_OS_HAS_DLOPEN
            BOTAN_TARGET_OS_HAS_GMTIME_R
            BOTAN_TARGET_OS_HAS_POSIX_MLOCK
            BOTAN_HAS_DYNAMICALLY_LOADED_ENGINE
            BOTAN_HAS_DYNAMIC_LOADER
    )
endif()

if(WIN32)
    target_compile_definitions(QSsh
        PRIVATE
            BOTAN_TARGET_OS_IS_WINDOWS
            BOTAN_TARGET_OS_HAS_LOADLIBRARY
            BOTAN_TARGET_OS_HAS_WIN32_GET_SYSTEMTIME
            BOTAN_TARGET_OS_HAS_WIN32_VIRTUAL_LOCK
            BOTAN_HAS_DYNAMICALLY_LOADED_ENGINE
            BOTAN_HAS_DYNAMIC_LOADER
            BOTAN_HAS_ENTROPY_SRC_CAPI
            BOTAN_HAS_ENTROPY_SRC_WIN32
            BOTAN_HAS_MUTEX_WIN32
    )
    target_link_libraries(QSsh PRIVATE advapi32 user32)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_definitions(QSsh PRIVATE BOTAN_BUILD_COMPILER_IS_GCC)
    target_compile_options(QSsh PRIVATE -fPIC -fpermissive -finline-functions -Wno-long-long)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_definitions(QSsh PRIVATE BOTAN_BUILD_COMPILER_IS_CLANG)
    target_compile_options(QSsh PRIVATE -fPIC -fpermissive -finline-functions -Wno-long-long)
elseif(MSVC)
    target_compile_definitions(QSsh
        PRIVATE
            BOTAN_BUILD_COMPILER_IS_MSVC
            BOTAN_TARGET_OS_HAS_GMTIME_S
            _SCL_SECURE_NO_WARNINGS
    )
    target_compile_options(QSsh PRIVATE /wd4251 /wd4290 /wd4250 /wd4068)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
    target_compile_definitions(QSsh PRIVATE BOTAN_TARGET_ARCH_IS_X86_64)
endif()

target_link_libraries(QSsh
    PUBLIC
        Qt5::Core
        Qt5::Network
        Qt5::Gui
        Qt5::Widgets
    PRIVATE
        Threads::Threads
        ${CMAKE_DL_LIBS}
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(QSsh PRIVATE rt)
endif()

set_target_properties(QSsh PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${FILETRANSFERKIT_LIB_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${FILETRANSFERKIT_LIB_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${FILETRANSFERKIT_LIB_DIR}
)

add_library(FileTransferKit STATIC
    ${FILETRANSFERKIT_ROOT}/src/FileTransferClient.cpp
    ${FILETRANSFERKIT_QFTP_ROOT}/qftp.cpp
    ${FILETRANSFERKIT_QFTP_ROOT}/qurlinfo.cpp
)

target_include_directories(FileTransferKit
    PUBLIC
        ${FILETRANSFERKIT_ROOT}/src
        ${FILETRANSFERKIT_QSFTP_ROOT}/include
    PRIVATE
        ${FILETRANSFERKIT_QFTP_ROOT}
)

target_link_libraries(FileTransferKit
    PUBLIC
        Qt5::Core
        Qt5::Network
        Qt5::Gui
        Qt5::Widgets
        QSsh
)

set_target_properties(FileTransferKit PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${FILETRANSFERKIT_LIB_DIR}
)

add_executable(filetransferclienttest
    ${FILETRANSFERKIT_ROOT}/examples/filetransferclienttest/main.cpp
)

target_link_libraries(filetransferclienttest PRIVATE FileTransferKit)

set_target_properties(filetransferclienttest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${FILETRANSFERKIT_BIN_DIR}
)
