cmake_minimum_required(VERSION 3.30)

project(ptx_runner)

set(CUDAToolkit_ROOT /opt/cuda)
set(CMAKE_CUDA_ARCHITECTURES 89)
set(CMAKE_CUDA_COMPILER /opt/cuda/bin/nvcc)
set(CUDAHOSTCXX /usr/bin/gcc-13)
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -ccbin /usr/bin/gcc-13")

include_directories(/usr/local/cuda/include)
link_directories(/usr/local/cuda/lib64)

include_directories(/opt/cuda/include)
link_directories(/opt/cuda/lib64)

add_executable(${PROJECT_NAME} main.c)

target_link_libraries(
    ${PROJECT_NAME}
    nvptxcompiler_static
    m
    cuda
)

