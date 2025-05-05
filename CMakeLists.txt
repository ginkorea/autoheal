cmake_minimum_required(VERSION 3.30)
project(autoheal C)

set(CMAKE_C_STANDARD 99)

add_executable(autoheal main.c)

# Link math statically
target_link_libraries(autoheal m)

# Force static linking on supported systems
set_target_properties(autoheal PROPERTIES
        LINK_SEARCH_START_STATIC TRUE
        LINK_SEARCH_END_STATIC TRUE
)

# Optional: Set full static build for glibc/musl systems
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
