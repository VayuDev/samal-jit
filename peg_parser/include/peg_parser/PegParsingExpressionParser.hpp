#pragma once
#include "peg_parser/PegParser.hpp"
#include <cassert>

namespace peg {

sp<ParsingExpression> stringToParsingExpression(const std::string_view&);

// Just for the tests here
class ExpressionTokenizer {
public:
    explicit ExpressionTokenizer(const std::string_view& expr);
    [[nodiscard]] inline const std::optional<std::string>& currentToken() const {
        return mCurrentToken;
    }
    void advance();

private:
    void genNextToken();
    void consumeNonTerminal();
    std::string consumeString(char start, char end);
    char getCurrentChar();
    char getNextChar();
    bool skipWhitespaces();

    size_t mOffset = 0;
    const std::string_view mExprString;
    std::optional<std::string> mCurrentToken;
};

}