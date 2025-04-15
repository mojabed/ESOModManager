cmake_minimum_required(VERSION 3.16)
project(esomm LANGUAGES CXX)

include(qt.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${PROJECT_SOURCE_DIR}/ui)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR}
    COMPONENTS
        Core
        Gui
        Widgets
)
qt_standard_project_setup()

set(ESOMM_SOURCES
    src/esomm.cpp
    src/main.cpp
    ui/esomm.ui
    include/esomm.h
)

qt_add_executable(esomm)

set_target_properties(esomm
    PROPERTIES
        WIN32_EXECUTABLE TRUE
)

if(WIN32)
    target_sources(esomm PRIVATE esomm.rc)
endif()
     

target_sources(esomm
    PUBLIC
        ${ESOMM_SOURCES}
        ${RESOURCES}
)

target_link_libraries(esomm
    PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

target_include_directories(esomm
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/ui
        ${PROJECT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
)

#[[
target_precompile_headers(esomm
    PUBLIC
        ${PROJECT_SOURCE_DIR}/stdafx.h
)
]]

if(WIN32)
    get_target_property(_qmake_executable Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    add_custom_command(TARGET esomm POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            --no-compiler-runtime
            --no-translations
            --no-system-d3d-compiler
            "$<TARGET_FILE:esomm>"
        COMMENT "Running windeployqt..."
    )
endif()

add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(ui)