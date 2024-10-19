#include "../include/trie.hpp"

void TrieI::Add(const char* str, uint64_t id, uint64_t string_no) {
    NodeI* cur_v = root_.get(); 
    for (int i = 0; i != std::strlen(str); ++i) {
        char c = str[i];
        if ((c - '!') >= kAlphLen || (c - '!') < 0) {
            return;
        }
        NodeI* next_node = nullptr;
        if (cur_v->next.find(c - '!') == cur_v->next.end()) {
            nodes_.push_back(std::unique_ptr<NodeI>(new NodeI));
            ++node_count;
            cur_v->next[c - '!'] = nodes_.size() - 1;
            next_node = nodes_[nodes_.size() - 1].get();
        }
        if (next_node == nullptr) {
            cur_v = nodes_[cur_v->next[c - '!']].get();
        } else {
            cur_v = next_node;
        }
    }
    cur_v->terminal = true;
    if(cur_v->docs.size() == 0 || cur_v->docs[cur_v->docs.size() - 1].id != id) {
        cur_v->docs.push_back({id , 1, {string_no}});
        return;
    }
    ++cur_v->docs[cur_v->docs.size() - 1].freq;
    auto& lines = cur_v->docs[cur_v->docs.size() - 1].string_no;
    if (lines[lines.size() - 1] != string_no) {
        lines.push_back(string_no);
    }
}



std::shared_ptr<BuilderType> TrieI::Serialize() {
    auto builder = std::shared_ptr<BuilderType>(new BuilderType);

    std::vector<DocType> docs;
    for (auto& doc : root_->docs) {
        docs.push_back(CreateDoc(*builder, doc.id, doc.freq, builder->CreateVector(doc.string_no)));
    }

    std::vector<MapType> map_vector;
    for (auto& pair : root_->next) {
        map_vector.push_back(CreateNextMap(*builder, pair.first, pair.second));
    }

    NodeType root = CreateNode(*builder,
                                   builder->CreateVector(docs),
                                   builder->CreateVectorOfSortedTables(&map_vector),
                                   root_->terminal);

    std::vector<NodeType> nodes;
    for (auto& node : nodes_) {
        std::vector<DocType> docs;
        for (auto& doc : node->docs) {
            docs.push_back(CreateDoc(*builder, doc.id, doc.freq, builder->CreateVector(doc.string_no)));
        }

        std::vector<MapType> map_vector;
        for (auto& pair : node->next) {
            map_vector.push_back(CreateNextMap(*builder, pair.first, pair.second));
        }
    
        nodes.push_back(CreateNode(*builder,
                                   builder->CreateVector(docs),
                                   builder->CreateVectorOfSortedTables(&map_vector),
                                   node->terminal));
    }

    builder->Finish(CreateTrie(*builder, builder->CreateVector(nodes), root));

    return builder;
} 

void TrieI::PrintInfo(const char* s) {
    NodeI* cur_v = root_.get();
    for (int i = 0; i != std::strlen(s); ++i) {
        char c = s[i];
        if (cur_v->next.find(c - '!') == cur_v->next.end()) {
            std::cout << "NotFound\n";
            return;
        }
        cur_v = nodes_[cur_v->next[c - '!']].get();
    }
    if (cur_v->terminal) {
        std::cout << s << " freq " << cur_v->docs[0].freq << " line " << cur_v->docs[0].string_no[0] << '\n';
        return;
    }

    std::cout << "Not found\n";
    
}

