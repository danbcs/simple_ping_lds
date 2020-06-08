# Compila em: build/ping_lds
cmake_minimum_required(VERSION 3.13)  
project(ping_lds)

# Enable c++11 standard
set(CMAKE_CXX_STANDARD 11)  
          
# SET build
set(CMAKE_BINARY_DIR build/)

# Add main.cpp file of project root directory as source file
set(SOURCE_FILES src/sources/main.cpp)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(ping_lds ${SOURCE_FILES})

