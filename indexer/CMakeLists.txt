add_library(indexer include/indexer.hpp src/indexer.cpp
                    include/trie.hpp src/trie.cpp)

set_target_properties(indexer PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(indexer PRIVATE flatbuffers)