cmake_minimum_required(VERSION 3.17)

project(LaggyMultiplayerPeer CXX)

include_directories(godot-cpp/include)
include_directories(godot-cpp/gdextension)
include_directories(godot-cpp/gen/include)

add_library(laggy_multiplayer_peer SHARED
        src/laggy_multiplayer_peer.cpp
        src/laggy_multiplayer_peer.h
        src/laggy_packet.h
        src/laggy_packet.cpp
        src/register_types.cpp
        src/variant_utils.h
        src/variant_utils.cpp
)
