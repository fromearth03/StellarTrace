cmake_minimum_required(VERSION 4.0)
project(StellarTrace)

set(CMAKE_CXX_STANDARD 20)

add_executable(StellarTrace main.cpp
        Jsonnormalization.cpp
        include/Lexicon.hpp
        include/Lexiconfolder.hpp)

# Include path for json.hpp
target_include_directories(StellarTrace PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/external/json/single_include/nlohmann)
