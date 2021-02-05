cmake_minimum_required(VERSION 3.1)
project(epic_poold)

include_directories("rapidjson/include")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(OpenSSL 1.1 REQUIRED)

file(GLOB SRCFILES *.cpp)
add_executable(epic_poold ${SRCFILES})

target_link_libraries(epic_poold Threads::Threads) #${OPENSSL_LIBRARIES})
