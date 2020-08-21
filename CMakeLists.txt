cmake_minimum_required(VERSION 3.17)
project(contracts_internal)

set(CMAKE_CXX_STANDARD 17)

add_library(${PROJECT_NAME} include/database/SQLSession.h src/database/SQLSession.cpp include/database/QueryResult.h src/database/QueryResult.cpp include/database/SQLException.h src/database/SQLException.cpp include/database/queryConstructions.h src/database/queryConstructions.cpp)

target_link_libraries(${PROJECT_NAME} odbc32)
