cmake_minimum_required(VERSION 3.17)
project(contracts_internal)

set(CMAKE_CXX_STANDARD 17)

add_library(${PROJECT_NAME} include/database/SQLSession.h src/database/SQLSession.cpp include/database/QueryResult.h src/database/QueryResult.cpp include/database/SQLException.h src/database/SQLException.cpp include/database/queryConstructions.h src/database/queryConstructions.cpp include/database/SQLSafeHandle.h src/database/SQLSafeHandle.cpp include/Network include/networking/TCPSocket.h include/networking/NetworkMessage.h src/networking/TCPSocket.cpp src/networking/NetworkMessage.cpp include/networking/NetworkTranslator.h src/networking/NetworkTranslator.cpp include/networking/Protocol.h src/networking/Protocol.cpp)

target_link_libraries(${PROJECT_NAME} odbc32 Ws2_32)
