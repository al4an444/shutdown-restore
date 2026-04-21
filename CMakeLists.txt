cmake_minimum_required(VERSION 3.15)
project(ShutdownRestoreService LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(ShutdownRestoreService
    src/main.cpp
    src/service.cpp
    src/restore_point.cpp
    src/logger.cpp
)

target_link_libraries(ShutdownRestoreService PRIVATE
    ole32
    advapi32
    wbemuuid
    oleaut32
)

set_target_properties(ShutdownRestoreService PROPERTIES
    VS_MANIFEST_UAC_EXECUTION_LEVEL "requireAdministrator"
)
