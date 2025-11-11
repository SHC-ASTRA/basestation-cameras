cmake_minimum_required(VERSION 3.24)

project(basestation-cameras VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0)

add_executable(${PROJECT_NAME}
  src/main.cpp
  src/MainWindow.h
  src/MainWindow.cpp
  src/VideoFrameWidget.h
  src/VideoFrameWidget.cpp
  src/RtspStream.h
  src/RtspStream.cpp
  src/IpcServer.h
  src/IpcServer.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${GSTREAMER_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  Qt6::Core
  Qt6::Widgets
  Qt6::Network
  ${GSTREAMER_LIBRARIES}
)

target_compile_options(${PROJECT_NAME} PRIVATE
  ${GSTREAMER_CFLAGS_OTHER}
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

# Desktop integration (generate .desktop at build time to ensure availability)
set(APP_DESKTOP_FILE "${CMAKE_CURRENT_BINARY_DIR}/basestation-cameras.desktop")
file(WRITE "${APP_DESKTOP_FILE}" "[Desktop Entry]\n"
  "Type=Application\n"
  "Name=Basestation Cameras\n"
  "GenericName=RTSP Grid Viewer\n"
  "Comment=View multiple RTSP streams in a tiled grid\n"
  "Exec=basestation-cameras\n"
  "Icon=basestation-cameras\n"
  "Terminal=false\n"
  "Categories=AudioVideo;Video;Viewer;\n"
  "StartupWMClass=basestation-cameras\n"
  "X-GNOME-UsesNotifications=false\n")
install(FILES "${APP_DESKTOP_FILE}" DESTINATION share/applications)

