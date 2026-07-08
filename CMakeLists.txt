cmake_minimum_required(VERSION 3.20)

project(sealkey VERSION 0.6.1 LANGUAGES CXX)

include(GNUInstallDirs)

option(USE_SYSTEM_FLTK "Use an installed FLTK package" ON)
option(BUILD_STATIC "Prefer static linking where practical" ON)
option(ENABLE_TESTS "Build unit tests" ON)
option(BUILD_GUI "Build the FLTK desktop application" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SEALKEY_WINDOWS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/sealkey_icon_windows.ico")
set(SEALKEY_MAC_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/sealkey_icon_mac.icns")

add_library(sealkey_core
    src/AppInfo.cpp
    src/CryptoService.cpp
    src/GpgExecutable.cpp
    src/GpgProcess.cpp
    src/IniFile.cpp
    src/KeyStore.cpp
    src/SealKeyModels.cpp
    src/SealKeyPaths.cpp
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
    set(SEALKEY_GUI_SOURCES
        src/main.cpp
        src/MainWindow.cpp
    )

    if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/windows/sealkey.rc")
        enable_language(RC)
        list(APPEND SEALKEY_GUI_SOURCES resources/windows/sealkey.rc)
    endif()

    if(APPLE AND EXISTS "${SEALKEY_MAC_ICON}")
        list(APPEND SEALKEY_GUI_SOURCES "${SEALKEY_MAC_ICON}")
        set_source_files_properties("${SEALKEY_MAC_ICON}" PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )
    endif()

    if(USE_SYSTEM_FLTK)
        find_package(FLTK REQUIRED)
    else()
        set(FLTK_BUILD_GL OFF CACHE BOOL "" FORCE)
        set(FLTK_USE_LIBDECOR_GTK OFF CACHE BOOL "" FORCE)
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

    add_executable(sealkey ${SEALKEY_GUI_SOURCES})

    set_target_properties(sealkey PROPERTIES
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_ICON_FILE "sealkey_icon_mac"
        MACOSX_BUNDLE_BUNDLE_NAME "SealKey"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.sealkey.sealkey"
    )

    if(APPLE)
        add_custom_command(TARGET sealkey POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E touch "$<TARGET_BUNDLE_DIR:sealkey>"
            VERBATIM
        )
    endif()

    target_include_directories(sealkey PRIVATE include)
    target_link_libraries(sealkey PRIVATE sealkey_core)
    target_compile_definitions(sealkey PRIVATE
        SEALKEY_OBJECT_ICONS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/icons/objets"
        SEALKEY_OBJECT_ICONS_INSTALLED_DIR="${CMAKE_INSTALL_FULL_DATADIR}/sealkey/icons/objets"
    )

    if(UNIX AND NOT APPLE)
        target_compile_definitions(sealkey PRIVATE
            SEALKEY_SOURCE_ICON_PATH="${CMAKE_CURRENT_SOURCE_DIR}/icons/linux_icons/sealkey_256x256.png"
            SEALKEY_INSTALLED_ICON_PATH="${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/256x256/apps/sealkey.png"
        )
    endif()

    if(TARGET fltk::fltk)
        target_link_libraries(sealkey PRIVATE fltk::fltk)
    elseif(TARGET fltk)
        target_link_libraries(sealkey PRIVATE fltk)
    else()
        target_include_directories(sealkey PRIVATE ${FLTK_INCLUDE_DIR})
        target_link_libraries(sealkey PRIVATE ${FLTK_LIBRARIES})
    endif()

    if(TARGET fltk::images)
        target_link_libraries(sealkey PRIVATE fltk::images)
    elseif(TARGET fltk_images)
        target_link_libraries(sealkey PRIVATE fltk_images)
    elseif(FLTK_IMAGES_LIBRARIES)
        target_link_libraries(sealkey PRIVATE ${FLTK_IMAGES_LIBRARIES})
    elseif(FLTK_IMAGES_LIBRARY)
        target_link_libraries(sealkey PRIVATE ${FLTK_IMAGES_LIBRARY})
    endif()

    if(MSVC)
        target_compile_options(sealkey PRIVATE /W4)
    else()
        target_compile_options(sealkey PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    install(TARGETS sealkey
        BUNDLE DESTINATION .
        RUNTIME DESTINATION bin
    )

    if(UNIX AND NOT APPLE)
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/linux/sealkey.desktop.in"
            "${CMAKE_CURRENT_BINARY_DIR}/sealkey.desktop"
            @ONLY
        )
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/sealkey.desktop"
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
        )
        install(FILES icons/linux_icons/sealkey.svg
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
            RENAME sealkey.svg
        )
        install(FILES icons/linux_icons/sealkey_16x16.png
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/16x16/apps
            RENAME sealkey.png
        )
        install(FILES icons/linux_icons/sealkey_32x32.png
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/32x32/apps
            RENAME sealkey.png
        )
        install(FILES icons/linux_icons/sealkey_48x48.png
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps
            RENAME sealkey.png
        )
        install(FILES icons/linux_icons/sealkey_128x128.png
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps
            RENAME sealkey.png
        )
        install(FILES icons/linux_icons/sealkey_256x256.png
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps
            RENAME sealkey.png
        )
    endif()

    if(APPLE)
        add_custom_command(TARGET sealkey POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_directory
                    "${CMAKE_CURRENT_SOURCE_DIR}/icons/objets"
                    "$<TARGET_BUNDLE_DIR:sealkey>/Contents/Resources/icons/objets"
            VERBATIM
        )
    else()
        add_custom_command(TARGET sealkey POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_directory
                    "${CMAKE_CURRENT_SOURCE_DIR}/icons/objets"
                    "$<TARGET_FILE_DIR:sealkey>/icons/objets"
            VERBATIM
        )
    endif()

    install(DIRECTORY icons/objets
        DESTINATION ${CMAKE_INSTALL_DATADIR}/sealkey/icons
    )
endif()

if(ENABLE_TESTS)
    enable_testing()
    add_executable(sealkey_tests
        tests/test_gpg_arguments.cpp
        tests/test_ini.cpp
        tests/test_key_parser.cpp
        tests/test_sealkey_paths.cpp
    )
    target_link_libraries(sealkey_tests PRIVATE sealkey_core)
    add_test(NAME sealkey_tests COMMAND sealkey_tests)
endif()
