set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(server server_main.cpp server.cpp orders.cpp connection.cpp)
add_executable(client client_main.cpp client.cpp orders.cpp)

target_include_directories(server PRIVATE "${CMAKE_SOURCE_DIR}"
                                          "${CMAKE_SOURCE_DIR}/lib")
target_include_directories(client PRIVATE "${CMAKE_SOURCE_DIR}"
                                          "${CMAKE_SOURCE_DIR}/lib")

target_link_libraries(server PRIVATE util)
target_link_libraries(client PRIVATE util)
