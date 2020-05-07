add_library(tcp_service tcp_service.cpp)
target_include_directories(tcp_service PUBLIC ../inc)
target_include_directories(tcp_service PUBLIC ${CMAKE_SOURCE_DIR}/net/tcp_client/inc)