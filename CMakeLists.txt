cmake_minimum_required(VERSION 3.10)
project(LLM_FILE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH /opt/local)
set(PKG_CONFIG_PATH /opt/local/lib/pkgconfig)

find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER REQUIRED poppler-cpp)
pkg_check_modules(LIBZIP REQUIRED libzip)
pkg_check_modules(CURL REQUIRED libcurl)

add_executable(indexer
    src/main.cpp
    src/indexing/file_scanner.cpp
    src/indexing/metadata_extractor.cpp
    src/indexing/type_classifier.cpp
    src/indexing/content_extractor.cpp
    src/indexing/chunker.cpp
    src/embedding/embedder.cpp
    src/storage/vector_store.cpp
    src/agents/query_agent.cpp
    src/agents/search_agent.cpp
    src/agents/ranking_agent.cpp
    src/agents/orchestrator.cpp
    src/agents/image_agent.cpp
    src/server/search_server.cpp
)

target_include_directories(indexer PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${POPPLER_INCLUDE_DIRS}
    ${LIBZIP_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    /opt/local/include
)

target_link_directories(indexer PRIVATE
    /opt/local/lib
)

target_link_libraries(indexer
    ${POPPLER_LIBRARIES}
    ${LIBZIP_LIBRARIES}
    ${CURL_LIBRARIES}
)