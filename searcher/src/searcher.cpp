#include "../include/searcher.hpp"

std::string StringProc(char* start, size_t len) {
    std::string_view word(start, len);
    std::string str(static_cast<std::string>(word));
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); });
    
    return str;
}

std::vector<std::string> ParseQuery(char* query) {
    
    std::vector<std::string> tokens;
    std::string_view sv;
    char* start_word = query;
    size_t len = 0;
    for (int i = 0; i != std::strlen(query); ++i) {
        if (query[i] == ' ') {
            if (len > 0) {
                tokens.push_back(std::move(StringProc(start_word, len)));
            }
            start_word += len + 1;
            len = 0;
        } else if (query[i] == '(') {
            if (len > 0) {
                tokens.push_back(std::move(StringProc(start_word, len)));
            }
            tokens.push_back("(");
            start_word += len + 1;
            len = 0;
        } else if (query[i] == ')') {
            if (len > 0) {
                tokens.push_back(std::move(StringProc(start_word, len)));
            }
            tokens.push_back(")");
            start_word += len + 1;
            len = 0;
        } else {
            ++len;
        }
    }
    if (len > 0) {
        tokens.push_back(std::move(StringProc(start_word, len)));
    }

    return tokens;
}

double GetScoreSingle(int freq, int doc_length, int ave_length, long long doc_count_gen, int doc_count_term) {
    const int k = 2;
    const int b = 1;

    double res = double(freq * (k + 1)) / (freq + k * (1 - b + b * (double(doc_length) / ave_length))) * 
                log(double(doc_count_gen) / doc_count_term);

    return res;
    
}

std::vector<uint64_t> GetOrVector(const std::vector<uint64_t>& l, const std::vector<uint64_t>& r) {
    int lp, rp; lp = rp = 0;
    std::vector<uint64_t> res;
    while (lp != l.size() && rp != r.size()) {
        if (l[lp] == r[rp]) {
            res.push_back(l[lp]);
            ++lp; ++rp;
            continue;
        }
        if (l[lp] < r[rp]) {
            res.push_back(l[lp]);
            ++lp;
            continue;
        }
        res.push_back(r[rp]);
        ++rp;
    }
    
    if (lp == l.size()) {
        while (rp != r.size()) {
            res.push_back(r[rp]);
            ++rp;
        }
    } else {
        while (lp != l.size()) {
            res.push_back(l[lp]);
            ++lp;
        }
    }

    return res;
}

void push_doc_heap_two(std::vector<DocI>& res, DocI& res_doc, const DocI& left_doc, const DocI& right_doc) {
    res_doc.string_no = std::move(GetOrVector(left_doc.string_no, right_doc.string_no));
    res_doc.id = left_doc.id;
    res.push_back(res_doc);
}

std::vector<DocI> SyntaxTree::GetDocVector(const char* s) {
    auto nodes = trie_->nodes();
    auto root = trie_->root();
    auto cur_v = root;
    for (int i = 0; i != std::strlen(s); ++i) {
        char c = s[i];
        if (cur_v->next()->LookupByKey(c - '!') == nullptr) {
            return std::vector<DocI>{};
        }
        cur_v = nodes->Get(cur_v->next()->LookupByKey(c - '!')->ref());
    }
    
    std::vector<DocI> res;
    double min = std::numeric_limits<double>::max();
    int count = 0;
    for (int i = 0; i != cur_v->docs()->size(); ++i) {
        uint64_t id = cur_v->docs()->Get(i)->id();
        DocI doc{id, 
                GetScoreSingle(cur_v->docs()->Get(i)->freq(), 
                               docs_->doc_table()->LookupByKey(id)->word_count(),
                               ave_length_,
                               doc_count_,
                               cur_v->docs()->size())};
        auto strs = cur_v->docs()->Get(i)->string_no();
        for (int j = 0; j != strs->size(); ++j) {
            doc.string_no.push_back(strs->Get(j));
        }
        
        res.push_back(doc);
    }
    
    return res;
}

std::vector<DocI> SyntaxTree::GetVec(std::shared_ptr<Vertex> cur) {
    if (cur->type == NOP) {
        if ((cur->right != nullptr && (cur->right->type == AND || cur->right->type == OR)) || 
            (cur->left != nullptr && (cur->left->type == AND || cur->right->type == OR))) {
            throw std::runtime_error("Invalid query");
        }
        return GetDocVector(cur->value.c_str());
    }
    if (cur->type == AND) {
        if (cur->left == nullptr) {
            throw std::runtime_error("Invalid query!");
        }
        auto left_res = std::move(GetVec(cur->left));
        auto right_res = std::move(GetVec(cur->right));
        
        if (left_res.size() == 0) {
            return left_res;
        }
        if (right_res.size() == 0) {
            return right_res;
        }
        
        int rp, lp; rp = lp = 0;
        std::vector<DocI> res;
        while (lp != left_res.size() && rp != right_res.size()) {
            if (left_res[lp].id == right_res[rp].id) {
                DocI res_doc;
                res_doc.score = left_res[lp].score + right_res[rp].score;
                
                push_doc_heap_two(res, res_doc, left_res[lp], right_res[rp]);
                
                ++lp; ++rp;
                continue;
            }
            if (left_res[lp].id < right_res[rp].id) {
                ++lp;
                continue;
            }
            ++rp;
        }
        
        
        return res;
    }

    if (cur->left == nullptr) {
        throw std::runtime_error("Invalid query!");
    }
    auto left_res = std::move(GetVec(cur->left));
    auto right_res = std::move(GetVec(cur->right));
    int rp, lp; rp = lp = 0;
    std::vector<DocI> res;
    while (lp != left_res.size() && rp != right_res.size()) {
        if (left_res[lp].id == right_res[rp].id) {
            DocI res_doc;
            res_doc.score = left_res[lp].score + right_res[rp].score;
            
            push_doc_heap_two(res, res_doc, left_res[lp], right_res[rp]);
            
            ++lp; ++rp;

            continue;
        }
        if (left_res[lp].id < right_res[rp].id) {
            res.push_back(left_res[lp]);
            ++lp;
            continue;
        }
        res.push_back(right_res[rp]);
        ++rp;
    }
    if (lp == left_res.size()) {
        while (rp != right_res.size()) {
            res.push_back(right_res[rp]);
            ++rp;
        }
    } else {
        while (lp != left_res.size()) {
            res.push_back(left_res[lp]);
            ++lp;
        }
    }
    return res;
}

std::vector<DocI> SyntaxTree::CountRes(int k) {
    auto temp = std::move(GetVec(root_));
    std::make_heap(temp.begin(), temp.end(), [](const DocI& l, const DocI& r){return l.score < r.score;});
    std::vector<DocI> res;
    int size = temp.size();
    for (int i = 0; i != k && i < size; ++i) {
        std::pop_heap(temp.begin(), temp.end(), [](const DocI& l, const DocI& r){return l.score < r.score;});
        res.push_back(temp.back());
        temp.pop_back();
    }

    auto table = docs_->doc_table();
    for (int i = 0; i != res.size(); ++i) {
        res[i].filename = table->LookupByKey(res[i].id)->name()->c_str();
    }

    return res;
}

std::vector<DocI> SyntaxTree::GetResult(std::string&& query, int k) {
    std::vector<std::string> tokens = std::move(ParseQuery(const_cast<char*>(query.c_str())));
    
    if (tokens.size() == 3 && tokens[0] == "(" && tokens[tokens.size() - 1] == ")") {
        tokens = std::move(std::vector<std::string>{tokens[1]});
    } else if (tokens.size() != 1 && !(tokens[0] == "(" && tokens[tokens.size() - 1] == ")")) {
        std::vector<std::string> res{"("};
        res.insert(res.end(), std::make_move_iterator(tokens.begin()), std::make_move_iterator(tokens.end()));
        res.push_back(")");
        tokens = std::move(res);
    }
    
    std::shared_ptr<Vertex> cur = root_;
    for (int i = 0; i != tokens.size(); ++i) {
        if (cur == nullptr) {
            throw std::runtime_error("Invalid query");
        }
        if (tokens[i] == "(") {
            cur->left = std::make_shared<Vertex>(NOP, cur);
            cur = cur->left;
            continue;
        }
        if (tokens[i] == ")") {
            cur = cur->parent;
            continue;
        }
        if (tokens[i] == "or") {
            cur->type = OR;
            cur->right = std::make_shared<Vertex>(NOP, cur);
            cur = cur->right;
            continue;
        }
        if (tokens[i] == "and") {
            cur->type = AND;
            cur->right = std::make_shared<Vertex>(NOP, cur);
            cur = cur->right;
            continue;
        }
        cur->value = tokens[i];
        cur = cur->parent;
    }
    
    return CountRes(k);
}

void SyntaxTree::PrintTop(std::string&& query, int k) {
    
    std::vector<DocI> top = std::move(GetResult(std::forward<std::string&&>(query), k));
    
    auto table = docs_->doc_table();
    for (int i = 0; i != k && i != top.size(); ++i) {
        std::cout << top[i].score << " ";
        std::cout << i + 1 << ") ";
        std::cout << table->LookupByKey(top[i].id)->name()->c_str() << '\n';
        std::cout << "In lines: ";
        for (int j = 0; j != top[i].string_no.size() - 1; ++j) {
            std::cout << top[i].string_no[j] << ", ";
        }
        if (top[i].string_no.size() != 0) {
            std::cout << top[i].string_no[top[i].string_no.size() - 1];
        }
        std::cout << "\n\n";
    }
}