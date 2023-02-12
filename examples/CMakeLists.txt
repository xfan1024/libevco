add_executable(helloworld helloworld.cpp)
target_link_libraries(helloworld evco)

add_executable(echo_server echo_server.cpp utils.cpp)
target_link_libraries(echo_server evco)

add_executable(echo_client echo_client.cpp utils.cpp)
target_link_libraries(echo_client evco)
