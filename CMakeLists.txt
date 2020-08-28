cmake_minimum_required(VERSION 3.17)
project(contracts_internal)

set(CMAKE_CXX_STANDARD 17)

add_definitions(-DWIN32_LEAN_AND_MEAN)

add_subdirectory(${PROJECT_SOURCE_DIR}/../encrypt encrypt-build)
include_directories(${PROJECT_SOURCE_DIR}/../encrypt/include)

add_library(${PROJECT_NAME} include/database/SQLSession.h src/database/SQLSession.cpp include/database/QueryResult.h src/database/QueryResult.cpp include/database/SQLException.h src/database/SQLException.cpp include/database/queryConstructions.h src/database/queryConstructions.cpp include/database/SQLSafeHandle.h src/database/SQLSafeHandle.cpp include/Network include/networking/TCPSocket.h include/networking/NetworkMessage.h src/networking/TCPSocket.cpp src/networking/NetworkMessage.cpp include/networking/NetworkTranslator.h src/networking/NetworkTranslator.cpp include/networking/protocol/Protocol.h src/networking/protocol/Protocol.cpp src/networking/protocol/layers/KeyExchange.cpp include/networking/protocol/layers/KeyExchange.h include/networking/protocol/protocolInternal.h include/networking/protocol/layers/PrimitiveExchange.h include/networking/protocol/layers/RSAMessageLayer.h src/networking/protocol/layers/RSAMessageLayer.cpp src/networking/protocol/layers/AESMessageLayer.cpp include/networking/protocol/layers/AESMessageLayer.h)

target_link_libraries(${PROJECT_NAME} encrypt odbc32 Ws2_32)
