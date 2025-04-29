cmake_minimum_required(VERSION 3.15)
project(Cordactyl_Bot VERSION 2.0 DESCRIPTION "Central Cordatcyl Discord-Bot")

set(CMAKE_CXX_STANDARD 23)

# Add DPP as dependency
add_subdirectory(libs/DPP)

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/globals.hpp
        src/utilities.hpp
        src/signal_handler.cpp
        src/signal_handler.h
        src/class/acknowledge_payload.cpp
        src/class/acknowledge_payload.h
        src/class/queued_messages.cpp
        src/class/queued_messages.h
        src/class/translations.cpp
        src/class/translations.h
        src/class/json_wrapper.cpp
        src/class/json_wrapper.h
        src/commands/appeals.cpp
        src/commands/appeals.h
)

target_link_libraries(${PROJECT_NAME}
        dpp
)

# Specify includes
target_include_directories(${PROJECT_NAME} PRIVATE
        libs/DPP/include
)

set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
)