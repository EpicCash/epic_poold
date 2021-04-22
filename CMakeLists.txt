cmake_minimum_required(VERSION 3.1)
project(epic_poold)

include_directories("rapidjson/include")

set(include_dir ${PROJECT_SOURCE_DIR}/ethash/include)
add_subdirectory(ethash/lib)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -maes -msse4")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -maes -msse4")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(OpenSSL 1.1 REQUIRED)

enable_language(ASM)

file(GLOB randomx randomx/src/*.cpp randomx/src/*.c randomx/src/blake2/*.c randomx/src/*.S)
file(GLOB SRCFILES *.cpp)

add_executable(epic_poold ${SRCFILES} ${randomx})
target_include_directories(epic_poold PUBLIC randomx/src)
target_link_libraries(epic_poold Threads::Threads keccak ethash) #${OPENSSL_LIBRARIES})
