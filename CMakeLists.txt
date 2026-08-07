cmake_minimum_required(VERSION 3.20)

project(openmp_pairwise_distance VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenMP REQUIRED COMPONENTS CXX)

add_library(pairwise_distance src/pairwise_distance.cpp)
target_include_directories(pairwise_distance PUBLIC include)
target_link_libraries(pairwise_distance PUBLIC OpenMP::OpenMP_CXX)

add_executable(pairwise_distance_openmp app/main.cpp)
target_link_libraries(pairwise_distance_openmp PRIVATE pairwise_distance)

foreach(target pairwise_distance pairwise_distance_openmp)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()
endforeach()

include(CTest)
if(BUILD_TESTING)
    add_executable(pairwise_distance_tests tests/test_pairwise_distance.cpp)
    target_link_libraries(pairwise_distance_tests PRIVATE pairwise_distance)
    if(MSVC)
        target_compile_options(pairwise_distance_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(pairwise_distance_tests PRIVATE
                               -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()

    foreach(thread_count 1 2 4)
        add_test(NAME correctness_${thread_count}_thread
                 COMMAND pairwise_distance_tests ${thread_count})
        set_tests_properties(correctness_${thread_count}_thread
                             PROPERTIES ENVIRONMENT "OMP_NUM_THREADS=${thread_count}")
    endforeach()

    add_test(NAME cli_smoke
             COMMAND pairwise_distance_openmp
                     --generate 64 --seed 123 --geometry wraparound
                     --algorithm optimized --schedule guided --threads 2)
endif()
