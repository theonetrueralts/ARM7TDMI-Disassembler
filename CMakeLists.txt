cmake_minimum_required(VERSION 3.24)
project(ARM7TDMI LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

# --- engine sources & public headers
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
     src/disassembler/*.cpp)

file(GLOB_RECURSE PUBLIC_HEADERS CONFIGURE_DEPENDS include/totr/**/*.hpp)

add_library(ARM7TDMI_Decoder STATIC ${ENGINE_SOURCES} ${PUBLIC_HEADERS})

target_include_directories(ARM7TDMI_Decoder PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)

# ---  executable
add_executable(Disassembler src/main.cpp)
target_link_libraries(Disassembler PRIVATE ARM7TDMI_Decoder)
