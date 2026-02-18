cmake_minimum_required(VERSION 3.16)
project(VidMetric VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Source files
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/HistoryTab.cpp
    src/PredictTab.cpp
    src/VerifyTab.cpp
    src/VideoUtils.cpp
    src/AbAv1Job.cpp
    src/FfmpegJob.cpp
)

set(HEADERS
    src/MainWindow.h
    src/HistoryTab.h
    src/PredictTab.h
    src/VerifyTab.h
    src/VideoUtils.h
    src/AbAv1Job.h
    src/FfmpegJob.h
)

set(RESOURCES
    resources/application.qrc
)

# Windows icon
if(WIN32)
    set(APP_ICON_RESOURCE resources/app.rc)
endif()

# Create executable
if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${RESOURCES} ${APP_ICON_RESOURCE})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})
endif()

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Deploy Qt dependencies on Windows
if(WIN32)
    # Find windeployqt
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")
    
    if(WINDEPLOYQT_EXECUTABLE)
        # Run windeployqt after build
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --no-translations
                --no-system-d3d-compiler
                --no-opengl-sw
                "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Running windeployqt to deploy Qt dependencies..."
        )
    endif()
    
    # Copy ab-av1.exe from the source root to the output bin directory
    if(EXISTS "${CMAKE_SOURCE_DIR}/resources/ab-av1.exe")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/resources/ab-av1.exe"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copying ab-av1.exe to build directory..."
        )
    endif()
    
    # Copy MinGW runtime DLLs
    if(MINGW)
        get_filename_component(MINGW_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
        set(MINGW_DLLS
            "${MINGW_BIN_DIR}/libstdc++-6.dll"
            "${MINGW_BIN_DIR}/libgcc_s_seh-1.dll"
            "${MINGW_BIN_DIR}/libwinpthread-1.dll"
        )
        
        foreach(DLL ${MINGW_DLLS})
            if(EXISTS ${DLL})
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${DLL}"
                        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                    COMMENT "Copying ${DLL}"
                )
            endif()
        endforeach()
        
        # Copy ICU and other Qt dependencies from Qt bin directory
        set(QT_ADDITIONAL_DLLS
            "${QT_BIN_DIR}/libicuin*.dll"
            "${QT_BIN_DIR}/libicuuc*.dll"
            "${QT_BIN_DIR}/libicudt*.dll"
            "${QT_BIN_DIR}/libdouble-conversion.dll"
            "${QT_BIN_DIR}/libb2-*.dll"
            "${QT_BIN_DIR}/libpcre2-16-0.dll"
            "${QT_BIN_DIR}/libzstd.dll"
            "${QT_BIN_DIR}/libharfbuzz-0.dll"
            "${QT_BIN_DIR}/libpng16-16.dll"
            "${QT_BIN_DIR}/libfreetype-6.dll"
            "${QT_BIN_DIR}/libbz2-1.dll"
            "${QT_BIN_DIR}/libbrotlidec.dll"
            "${QT_BIN_DIR}/libbrotlicommon.dll"
            "${QT_BIN_DIR}/libgraphite2.dll"
            "${QT_BIN_DIR}/libmd4c.dll"
        )
        
        foreach(DLL_PATTERN ${QT_ADDITIONAL_DLLS})
            file(GLOB DLL_FILES "${DLL_PATTERN}")
            foreach(DLL ${DLL_FILES})
                if(EXISTS ${DLL})
                    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            "${DLL}"
                            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                        COMMENT "Copying Qt dependency: ${DLL}"
                    )
                endif()
            endforeach()
        endforeach()
    endif()
endif()
