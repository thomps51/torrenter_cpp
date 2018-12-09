project(Torrentor)
cmake_minimum_required(VERSION 2.8.12)
add_definitions("-std=c++17")

# Required packages:
# boost
# nng
# capnp
# libtorrent

include_directories(/home/athomps/libs/include)
link_directories(/home/athomps/libs/lib)

#include(/home/athomps/install/capnproto-c++-0.6.1/cmake/CapnProtoMacros.cmake)
#include(/home/athomps/install/capnproto-c++-0.6.1/cmake/FindCapnProto.cmake)
#include(/home/athomps/install/capnproto-c++-0.6.1/cmake/CapnProtoConfig.cmake.in)
#capnp_generate_cpp(CAPNP_SRCS CAPNP_HDRS add_torrent.proto)
#find_package(CapnProto REQUIRED)

find_package(Boost COMPONENTS system chrono thread REQUIRED)
find_package(nng CONFIG REQUIRED)
find_package(CURL REQUIRED)

#include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
#conan_basic_setup()

include_directories(/home/athomps/libraries/libtorrent/include)
link_directories(/home/athomps/libraries/libtorrent/bin/gcc-7/debug/threading-multi)

add_executable(torrentor 
    main.cpp
    torrenter.cpp
    torrent_manager.cpp
    vpn.cpp
    add_torrent.proto.c++)

target_link_libraries(torrentor ${CONAN_LIBS} -ltorrent ${Boost_SYSTEM_LIBRARY} ${Boost_CHRONO_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_PROCESS_LIBRARY} -lstdc++fs ${nng_LIBRARY} -lcapnp -lkj nng::nng -lcurl)
target_include_directories(torrentor PRIVATE "/home/athomps/dev")
