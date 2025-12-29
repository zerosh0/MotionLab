cmake_minimum_required(VERSION 3.15)
project(MotionLab C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(WIN32)
    add_definitions(-DNOMINMAX)
    set(RES_FILE "src/resources.rc")
    set(WIN_FLAGS WIN32)
endif()

include_directories(include)

find_package(OpenCV REQUIRED)

if(WIN32)
    find_package(raylib CONFIG REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(FFMPEG REQUIRED)
else()
    find_package(raylib REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavformat libswscale libavutil)
endif()


file(GLOB_RECURSE SOURCES "src/*.c" "src/*.cpp")

add_executable(MotionLab ${WIN_FLAGS} ${SOURCES} ${RES_FILE})

if(MSVC)
    target_link_options(MotionLab PRIVATE "/ENTRY:mainCRTStartup")
endif()

set_target_properties(MotionLab PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(MotionLab PRIVATE raylib ${OpenCV_LIBS})

if(WIN32)
    target_link_libraries(MotionLab PRIVATE 
        glfw
        ${FFMPEG_LIBRARIES}
        opengl32 user32 gdi32 winmm shell32 comdlg32 ole32 uuid setupapi advapi32 imm32
    )
    target_include_directories(MotionLab PRIVATE ${FFMPEG_INCLUDE_DIRS})
else()
    target_link_libraries(MotionLab PRIVATE 
        PkgConfig::FFMPEG
        m
        pthread
        dl
    )
endif()
