#pragma once

#include <memory>
#include <vector>
#include <cstring>
#include <iostream>
#include <string>
#include <algorithm>
#include <unordered_map>

#include "../../generated/trie_generated.h"

using NodeType = flatbuffers::Offset<Node>;
using BuilderType = flatbuffers::FlatBufferBuilder;
using DocType = flatbuffers::Offset<Doc>;
using MapType = flatbuffers::Offset<NextMap>;

const int kAlphLen = 90;

class TrieI {

    struct DocI {
        uint64_t id;
        uint64_t freq;
        std::vector<uint64_t> string_no;
    };

    struct NodeI {
        NodeI() 
            : terminal(false)
        {}
        std::unordered_map<char, uint64_t> next;
        std::vector<DocI> docs;
        bool terminal;
    };

public:
    TrieI()
        : root_(new NodeI)
    {}

    void Add (const char* str, uint64_t id, uint64_t string_no);
    void PrintInfo(const char* s);
    std::shared_ptr<BuilderType> Serialize();
    int node_count = 0;
private:
    std::string cur_str_;
    std::unique_ptr<NodeI> root_;
    std::vector<std::unique_ptr<NodeI>> nodes_;
    
};

