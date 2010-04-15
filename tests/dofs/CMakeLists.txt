if(NOT REAL)
    return()
endif(NOT REAL)
project(dofs)

add_executable(${PROJECT_NAME} main.cpp)
include (../CMake.common)

set(BIN ${PROJECT_BINARY_DIR}/${PROJECT_NAME})
add_test(dofs ${BIN})
set_tests_properties(dofs PROPERTIES LABELS slow)

