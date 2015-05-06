cmake_minimum_required(VERSION 2.8)
find_package(Protobuf REQUIRED)
set( GEN_LUA_INCLUDE_DIRS
  	"${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${PROTOBUF_INCLUDE_DIR}"
)

file(GLOB GEN_LUA_SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

set(ALL_SRC_FILES
    ${GEN_LUA_SRC_FILES}
)


include_directories(${GEN_LUA_INCLUDE_DIRS})
add_executable(protoc_gen_lua ${ALL_SRC_FILES})
target_link_libraries(protoc_gen_lua debug ${PROTOBUF_LIBRARY_DEBUG} ${PROTOBUF_PROTOC_LIBRARY_DEBUG})
target_link_libraries(protoc_gen_lua optimized ${PROTOBUF_LIBRARY} ${PROTOBUF_PROTOC_LIBRARY})