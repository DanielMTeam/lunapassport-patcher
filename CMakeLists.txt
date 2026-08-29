cmake_minimum_required(VERSION 3.10)
project(lunapassport-patcher CXX RC)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Single portable .exe: static CRT and no extra DLLs.
if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++ -mwindows")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -municode")
    set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} --use-temp-file")
elseif(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_executable(lunapassport-patcher WIN32
    src/main.cpp
    src/app.cpp
    src/hosts.cpp
    src/registry.cpp
    src/cert.cpp
    src/util.cpp
    res/app.rc
)

target_include_directories(lunapassport-patcher PRIVATE src res)
target_compile_definitions(lunapassport-patcher PRIVATE
    _WIN32_WINNT=0x0501
    WINVER=0x0501
    UNICODE
    _UNICODE
)

target_link_libraries(lunapassport-patcher PRIVATE
    comctl32
    crypt32
    advapi32
    shell32
    user32
    gdi32
)

set_target_properties(lunapassport-patcher PROPERTIES
    OUTPUT_NAME "lunapassport-patcher"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
