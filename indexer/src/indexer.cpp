#include "../include/indexer.hpp"

using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;


Indexer::Indexer(const std::filesystem::path& dir_path) 
    : dir_path_(dir_path)
    , gen_word_count_(0)
    , doc_count_(0)
    , non_text_files_({
        "jpeg", "png", "gif", "heif", "jpg", "bmp", "webp", "tif", "djvu", "heic", "svg", "svgz", "ai", 
        "aac", "mp3", "wav", "flac",
        "amv", "mpeg", "flv", "avi", "mp4", "mov", "ts", 
        "iso", "rar", "tar", "7z", "zip", "gz", "z", "jar",
        "pdf", "doc", "docx", "odt", "pub", "fb2", "epub", "pptx", "odp", "ppt", "ods", 
        "sql", "db", "pdb", 
        "exe", "scr", "ipa", "cmd", "o", "file", "journal", "ipch", "bfbs", "bin", "mon"
    }
    )
    {}
void Indexer::Index(bool& run) {
    run = true;
    uint64_t id_count = 0;
    try {
    for (const auto& dirEntry : recursive_directory_iterator(dir_path_)) {
        if (std::filesystem::is_regular_file(dirEntry)) {
            std::string_view sv(dirEntry.path().c_str());
            sv.remove_prefix(std::min(sv.find_last_of(".") + 1, sv.size()));
            if (!non_text_files_.contains(sv.data())) {
                ++id_count;
                FileReader file(static_cast<std::string>(dirEntry.path()).c_str(), id_count);
                uint64_t word_count = file.ParseFile(bor_);
                if (word_count != 0) {
                    gen_word_count_ += word_count;
                    ++doc_count_;
                    file_ids_.insert({id_count, {word_count, std::filesystem::path(dirEntry)}});
                }
            }
        }
    }
    } catch (std::exception& e) {
        throw std::runtime_error("Invalid dir");
    }
    

    auto builder = bor_.Serialize();

    std::ofstream ofile_idx("serialized/idx.bin", std::ios::binary);
    ofile_idx.write(reinterpret_cast<char*>(builder->GetBufferPointer()), builder->GetSize());
    ofile_idx.close();

    auto builder_this = this->Serialize();

    std::ofstream ofile_this("serialized/docs.bin", std::ios::binary);
    ofile_this.write(reinterpret_cast<char*>(builder_this->GetBufferPointer()), builder_this->GetSize());
    ofile_this.close();

    std::ofstream dir_name_file("cur_dir.txt");
    dir_name_file << dir_path_.c_str();
    dir_name_file.close();

    run = false;
}

std::shared_ptr<BuilderType> Indexer::Serialize() {
    std::shared_ptr<BuilderType> builder = std::shared_ptr<BuilderType>(new BuilderType);
    std::vector<DocInfoType> doc_info;
    for (auto& doc : file_ids_) {
        doc_info.push_back(CreateDocInfo(*builder, doc.first, doc.second.first, builder->CreateString(doc.second.second.c_str())));
    }
    
    uint64_t word_ave = gen_word_count_ / doc_count_;
    builder->Finish(CreateIndex(*builder, gen_word_count_, word_ave, builder->CreateVectorOfSortedTables(&doc_info)));
    
    return builder;
}

bool FileReader::StringProc(char* start, uint64_t len, uint64_t line, TrieI& bor) {
    std::string_view word(start, len);
    word.remove_prefix(std::min(word.find_first_not_of("!#$%&()*,-./:;<=>?[\\]^{|}~'`\v\f\r\0"), word.size()));
    word.remove_suffix(std::min(word.size() - word.find_last_not_of("!#$%&()*,-./:;<=>?[\\]^{|}~'`\v\f\r\0") - 1, word.size()));

    std::string str(static_cast<std::string>(word));
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); });
    
    if (str != "") {
        bor.Add(str.c_str(), id_, line);

        return true;
    }

    return false;
}

uint64_t FileReader::ParseFile(TrieI& bor) {
    uint64_t len_word = 0;
    uint64_t line_number = 1;
    uint64_t word_count = 0;
    char* start_word = const_cast<char*>(contents_);
    for (uint64_t i = 0; i != file_len_; ++i) {
        unsigned char c = static_cast<unsigned char>(contents_[i]);
        if (isblank(c) || c == '\n' || c == '(' || c == ')' || c == '{' || c == '}' || c == '"' || c == '[' || c == ']' || c == '<' || c == '>') {
            if (len_word > 0) {
                StringProc(start_word, len_word, line_number, bor);
                ++word_count;
            }
            start_word += len_word + 1;
            len_word = 0;

            if (c == '\n') ++line_number;
        } else {
            ++len_word;
            if (len_word >= 50) {
                return word_count;
            }
        }  
    }
    if (len_word > 0) {
        if(StringProc(start_word, len_word, line_number, bor)) {
            ++word_count;
        }
    }

    return word_count;
}