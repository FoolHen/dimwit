cmake_minimum_required(VERSION 3.16)
project(Dimwit VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ddcutil REQUIRED ddcutil)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/main.h
    src/monitor_controller.cpp
    src/monitor_controller.h
    src/brightness_scheduler.cpp
    src/brightness_scheduler.h
    src/schedule_dialog.cpp
    src/schedule_dialog.h
    include/ddc_commander.cpp
    include/ddc_commander.h
    resources.qrc
)

target_include_directories(${PROJECT_NAME} PRIVATE include src)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets ${ddcutil_LIBRARIES})

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES icons/dimwit.png DESTINATION share/pixmaps)
install(FILES org.dimwit.app.desktop DESTINATION share/applications)