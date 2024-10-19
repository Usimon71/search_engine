# Filesystem Search Engine

## Description

[Full-text search](https://en.wikipedia.org/wiki/Full-text_search) over files from the computer's file system.

To perform the search, an [inverted search index](https://en.wikipedia.org/wiki/Inverted_index) was implemented (the index was serialized using the [flatbuffers](https://github.com/google/flatbuffers) library). Habr [article](https://habr.com/ru/articles/545634/) was taken as a basis

The problem of ranking (ordering documents found by request) has also been solved. The ranking function was used [BM25](https://en.wikipedia.org/wiki/Okapi_BM25), and as a parameter for it [TF-IDF](https://en.wikipedia.org/wiki/Tf%E2%80%93idf).

Thus, two applications were implemented - an indexer (preparing a search index) and a search (a program that directly searches the constructed index)

UI was implemented using [glfw](https://github.com/glfw/glfw) and [imgui](https://github.com/ocornut/imgui) libraries.

### Query syntax

Queries support parentheses as well as AND and OR operators.
Thus, logical expressions act as a request. Separator between words - space(s)

The following queries are considered valid

 - "for"
 - "vector OR list"
 - "vector AND list"
 - "(for)"
 - "(vector OR list)"
 - "(vector AND list)"
 - "(while OR for) and vector"
 - "for AND and"

Invalid requests are considered
 - "for AND"
 - "vector list"
 - "for AND OR list"
- "vector Or list"