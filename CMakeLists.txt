cmake_minimum_required(VERSION 3.20)

project(sealkey VERSION 0.2.0 LANGUAGES CXX)

option(USE_SYSTEM_FLTK "Use an installed FLTK package" ON)
option(BUILD_STATIC "Prefer static linking where practical" ON)
option(ENABLE_TESTS "Build unit tests" ON)
option(BUILD_GUI "Build the FLTK desktop application" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(sealkey_core
    src/AppInfo.cpp
    src/CryptoService.cpp
    src/GpgExecutable.cpp
    src/GpgProcess.cpp
    src/IniFile.cpp
    src/KeyStore.cpp
    src/SettingsService.cpp
    src/SignatureService.cpp
)

target_include_directories(sealkey_core PUBLIC include)

if(MSVC)
    target_compile_options(sealkey_core PRIVATE /W4)
else()
    target_compile_options(sealkey_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_GUI)
    if(USE_SYSTEM_FLTK)
        find_package(FLTK REQUIRED)
    else()
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/fltk/CMakeLists.txt")
            add_subdirectory(third_party/fltk EXCLUDE_FROM_ALL)
        else()
            include(FetchContent)
        FetchContent_Declare(
            fltk
            URL https://github.com/fltk/fltk/releases/download/release-1.4.1/fltk-1.4.1-source.tar.gz
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        set(FLTK_BUILD_TEST OFF CACHE BOOL "" FORCE)
        set(FLTK_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(FLTK_BUILD_FLUID OFF CACHE BOOL "" FORCE)
        set(OPTION_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        set(FLTK_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(fltk)
    endif()
endif()

    add_executable(sealkey
        src/main.cpp
        src/MainWindow.cpp
    )

    target_include_directories(sealkey PRIVATE include)
    target_link_libraries(sealkey PRIVATE sealkey_core)

    if(TARGET fltk::fltk)
        target_link_libraries(sealkey PRIVATE fltk::fltk)
    elseif(TARGET fltk)
        target_link_libraries(sealkey PRIVATE fltk)
    else()
        target_include_directories(sealkey PRIVATE ${FLTK_INCLUDE_DIR})
        target_link_libraries(sealkey PRIVATE ${FLTK_LIBRARIES})
    endif()

    if(MSVC)
        target_compile_options(sealkey PRIVATE /W4)
    else()
        target_compile_options(sealkey PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

if(ENABLE_TESTS)
    enable_testing()
    add_executable(sealkey_tests
        tests/test_gpg_arguments.cpp
        tests/test_ini.cpp
        tests/test_key_parser.cpp
    )
    target_link_libraries(sealkey_tests PRIVATE sealkey_core)
    add_test(NAME sealkey_tests COMMAND sealkey_tests)
endif()
