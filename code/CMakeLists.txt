cmake_minimum_required(VERSION 3.0)
project(opencvtest)

set(CMAKE_CXX_STANDARD 11)



add_executable(opencvtest q4.cpp)
# Where to find CMake modules and OpenCV
set(OpenCV_DIR  D:opencv/mingw-build)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})


## add libs you need
#set(OpenCV_LIBS opencv_core opencv_imgproc opencv_highgui opencv_imgcodecs opencv_objdetect opencv_video)
set(OpenCV_LIBS opencv_core opencv_imgproc opencv_highgui opencv_imgcodecs opencv_objdetect opencv_video)

#
## linking
target_link_libraries(opencvtest ${OpenCV_LIBS})
