#include <gtest/gtest.h>
#include "searcher/include/searcher.hpp"

TEST(SearcherTestSuitePos, SimpleTest) {
    SyntaxTree st;
    ASSERT_NO_THROW(st.GetResult("for", 3));
}

TEST(SearcherTestSuitePos, Test2) {
    SyntaxTree st;
    ASSERT_NO_THROW(st.GetResult("vector OR list", 3));
}

TEST(SearcherTestSuitePos, Test3) {
    SyntaxTree st;
    ASSERT_NO_THROW(st.GetResult("vector AND list", 5));
}

TEST(SearcherTestSuitePos, Test4) {
    SyntaxTree st;
    ASSERT_NO_THROW(st.GetResult("(for)", 5));
}

TEST(SearcherTestSuitePos, Test5) {
    SyntaxTree st;
    ASSERT_NO_THROW(st.GetResult("(vector OR list)", 10));
}

TEST(SearcherTestSuitePos, Test6) {
    SyntaxTree st;
    ASSERT_NO_THROW(st.GetResult("(vector AND list)", 10));
}

TEST(SearcherTestSuitePos, Test7) {
    SyntaxTree st;
    ASSERT_NO_THROW(st.GetResult("(while OR for) and vector", 3));
}

TEST(SearcherTestSuiteNeg, Test1) {
    SyntaxTree st;
    ASSERT_ANY_THROW(st.GetResult("vector list", 3));
}

TEST(SearcherTestSuiteNeg, Test2) {
    SyntaxTree st;
    ASSERT_ANY_THROW(st.GetResult("for AND OR list", 3));
}

TEST(SearcherTestSuiteNeg, Test3) {
    SyntaxTree st;
    ASSERT_ANY_THROW(st.GetResult("and And And", 3));
}