cmake_minimum_required(VERSION 3.16)
project(esomm LANGUAGES CXX)

include(qt.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${PROJECT_SOURCE_DIR}/ui)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6
    REQUIRED COMPONENTS
        Core
        Gui
        Widgets
        Network
        Concurrent
        WebEngineWidgets
)
qt_standard_project_setup()

set(RESOURCES
    resources.qrc
)

set(ESOMM_SOURCES
    src/main.cpp
    ui/esomm.ui

    ${RESOURCES}
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
)

if(MSVC)
    target_link_options(esomm PRIVATE "/SUBSYSTEM:CONSOLE")
    target_compile_options(esomm PRIVATE "/Zc:__cplusplus")
endif()
   

target_link_libraries(esomm
    PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Network
        Qt::Concurrent
        Qt::WebEngineWidgets
)

target_include_directories(esomm
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/ui
        ${PROJECT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
)

#if(WIN32)
#    get_target_property(_qmake_executable Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
#    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
#    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

#    add_custom_command(TARGET esomm POST_BUILD
#        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
#            --no-compiler-runtime
#            --no-translations
#            --no-system-d3d-compiler
#            "$<TARGET_FILE:esomm>"
#        COMMENT "Running windeployqt..."
#    )
#endif()

add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(ui)
add_subdirectory(resources)