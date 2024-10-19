add_library(searcher include/searcher.hpp src/searcher.cpp)

set_target_properties(searcher PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(searcher PRIVATE flatbuffers)