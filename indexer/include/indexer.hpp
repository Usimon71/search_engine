#pragma once

#include <filesystem>
#include <fstream>
#include <cctype>
#include <string_view>
#include <unordered_set>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <memory>

#include "trie.hpp"
#include "../../generated/index_generated.h"

class FileReader {
public:
    FileReader(const char* path, uint64_t id)
        : in_file_(open(path, O_RDONLY))
        , file_len_(lseek(in_file_, 0, SEEK_END))
        , contents_(static_cast<const char*>(mmap(NULL, file_len_, PROT_READ, MAP_SHARED, in_file_, 0)))
        , id_(id)
    {
        if (!in_file_) {
            std::cerr << "Unable to open file\n";
            exit(1);
        }
    }
    ~FileReader() {
        close(in_file_);
    }

    uint64_t ParseFile(TrieI& bor);
    bool StringProc(char* start, uint64_t len, uint64_t line, TrieI& bor);

private:
    int in_file_;
    size_t file_len_;
    uint64_t id_;
    const char* contents_;
};

using BuilderType = flatbuffers::FlatBufferBuilder;
using DocInfoType = flatbuffers::Offset<DocInfo>;

class Indexer {
    std::shared_ptr<BuilderType> Serialize();
public:
    Indexer(const std::filesystem::path& dir_path);
    void Index(bool& run);
private:
    std::filesystem::path dir_path_;
    TrieI bor_;
    std::unordered_map<uint64_t, std::pair<uint64_t, std::filesystem::path>> file_ids_;
    uint64_t gen_word_count_;
    uint64_t doc_count_;
    std::unordered_set<std::string> non_text_files_;
};