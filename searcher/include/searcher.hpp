#pragma once
#include "../../generated/index_generated.h"
#include "../../generated/trie_generated.h"

#include <memory>
#include <cstring>
#include <iostream>
#include <fstream>
#include <math.h>
#include <algorithm>
#include <limits>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

using DocType = flatbuffers::Offset<Doc>;
using VecDocType = flatbuffers::Vector<DocType>;

enum TypeOp {
    OR, AND, NOP
};

struct Vertex {
    TypeOp type;
    std::shared_ptr<Vertex> parent = nullptr;
    std::shared_ptr<Vertex> left = nullptr;
    std::shared_ptr<Vertex> right = nullptr;
    std::string value;
};

struct DocI {
    uint64_t id;
    double score;
    std::vector<uint64_t> string_no;
    const char* filename;
};

class SyntaxTree {
    std::vector<DocI> CountRes(int k);
    std::vector<DocI> GetVec(std::shared_ptr<Vertex> cur);
    std::vector<DocI> GetDocVector(const char* s);
    
public:
    SyntaxTree()
        : root_(new Vertex{NOP})
    {   
        int idx_file_ = open("serialized/idx.bin", O_RDONLY);
        size_t file_len = lseek(idx_file_, 0, SEEK_END);
        const char* content = static_cast<const char*>(mmap(NULL, file_len, PROT_READ, MAP_SHARED, idx_file_, 0));
        trie_ = GetTrie(content);
        
        int docs_file_ = open("serialized/docs.bin", O_RDONLY);
        size_t file_len2 = lseek(docs_file_, 0, SEEK_END);
        const char* content2 = static_cast<const char*>(mmap(NULL, file_len2, PROT_READ, MAP_SHARED, docs_file_, 0));
        docs_ = GetIndex(content2);

        doc_count_ = docs_->doc_table()->size();
        ave_length_ = docs_->word_ave();

    }
    void PrintTop(std::string&& query, int k);
    std::vector<DocI> GetResult(std::string&& query, int k);

    ~SyntaxTree() {
        close(idx_file_);
        close(docs_file_);
    }
private:
    std::shared_ptr<Vertex> root_;
    const Trie* trie_;
    const Index* docs_;
    int idx_file_;
    int docs_file_;
    long long doc_count_;
    long long ave_length_;
};