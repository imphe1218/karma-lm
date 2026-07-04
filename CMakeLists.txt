cmake_minimum_required(VERSION 3.16)
project(karma_lm C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(KARMA_BUILD_TESTS "Build KARMA-LM tests" ON)

add_library(karma_core
    src/karma_rng.c
    src/karma_hash.c
    src/karma_tensor.c
    src/karma_memory.c
    src/karma_model.c
)

target_include_directories(karma_core PUBLIC include)

if (MSVC)
    target_compile_options(karma_core PRIVATE /W4)
else()
    target_compile_options(karma_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(karma_demo src/main.c)
target_link_libraries(karma_demo PRIVATE karma_core m)

if (KARMA_BUILD_TESTS)
    add_executable(test_memory tests/test_memory.c)
    target_link_libraries(test_memory PRIVATE karma_core m)
endif()
