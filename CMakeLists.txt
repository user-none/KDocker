cmake_minimum_required(VERSION 3.18)

project(KDocker VERSION 5.99 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find and setup all dependency libraries
find_package(Qt6 REQUIRED COMPONENTS Core DBus Widgets)
qt_standard_project_setup()

find_package(X11 REQUIRED COMPONENTS Xmu xcb)

# Create some variables used when generating files
string(TIMESTAMP TIMESTAMP)
string(TIMESTAMP DATE "%d %B, %Y")
set(VERSION ${CMAKE_PROJECT_VERSION})

# install appdata xml
configure_file(resources/appdata/kdocker.appdata.xml.in ${CMAKE_CURRENT_BINARY_DIR}/appdata/kdocker.appdata.xml @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/appdata/kdocker.appdata.xml DESTINATION share/metainfo)

# Install icon
install(FILES resources/images/kdocker.png DESTINATION share/icons/hicolor/128x128/apps)

# Install desktop file
install(FILES resources/kdocker.desktop DESTINATION share/applications)

# Install bash completions
install(FILES resources/bash-completion/kdocker DESTINATION share/bash-completion/completions)

# Generate man page
find_program(POD2MAN pod2man)
if (NOT POD2MAN MATCHES "NOTFOUND")
    set(MAN_BASE_NAME "kdocker")
    set(POD_NAME "${MAN_BASE_NAME}.pod")
    set(POD_IN "${CMAKE_CURRENT_SOURCE_DIR}/resources/man/${POD_NAME}")
    set(MAN_OUT "${CMAKE_CURRENT_BINARY_DIR}/man/${MAN_BASE_NAME}.1")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/man/")
    add_custom_command(
        OUTPUT ${MAN_OUT}
        COMMAND ${POD2MAN} --center "General Commands Manual" --release "Version ${CMAKE_PROJECT_VERSION}" --date "${DATE}" "${POD_IN}" "${MAN_OUT}"
        VERBATIM)
    add_custom_target(generate_man ALL DEPENDS ${MAN_OUT})
    install(FILES ${MAN_OUT} DESTINATION share/man/man1)
else ()
    message(STATUS "pod2man not found")
    message(WARNING "will not generate man file")
endif ()

# Fill out some source files that needs variables added
configure_file(src/constants.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/constants.cpp @ONLY)

# Collect the source files
set(SOURCES
    src/application.cpp
    src/command.cpp
    src/commandlineargs.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/constants.cpp
    src/main.cpp
    src/scanner.cpp
    src/trayitem.cpp
    src/trayitemconfig.cpp
    src/trayitemmanager.cpp
    src/xlibutil.cpp
)

# Generate the dbus adaptor 
qt_add_dbus_adaptor(SOURCES resources/dbus/kdocker.xml trayitemconfig.h "" kdocker_adaptor)

# Add resources
qt_add_resources(SOURCES resources/resources.qrc)

# Build
qt_add_executable(kdocker ${SOURCES})

target_include_directories(kdocker PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(kdocker PRIVATE Qt6::Core Qt6::DBus Qt6::Widgets X11::X11 X11::Xmu X11::xcb)
