#include "peg_parser/PegParsingExpressionParser.hpp"
#include "peg_parser/PegUtil.hpp"

namespace peg {

static sp<ParsingExpression> parseChoice(ExpressionTokenizer& tok);

static sp<ParsingExpression> parseAtom(ExpressionTokenizer& tok) {
    if(!tok.currentToken())
        return nullptr;
    assert(!tok.currentToken()->empty());
    if(tok.currentToken()->at(0) == '\'') {
        auto expr = std::make_shared<TerminalParsingExpression>(std::string{ tok.currentToken()->substr(1, tok.currentToken()->size() - 2) });
        tok.advance();
        return expr;
    } else if(*tok.currentToken() == "(") {
        tok.advance();
        auto expr = parseChoice(tok);
        if(!tok.currentToken()) {
            throw std::runtime_error{ "Missing closing bracket" };
        }
        if(*tok.currentToken() != ")") {
            throw std::runtime_error{ "Expected closing bracket, not '" + std::string{ *tok.currentToken() } };
        }
        tok.advance();
        return expr;
    } else if(isalnum(tok.currentToken()->at(0))) {
        // non-terminal
        auto expr = std::make_shared<NonTerminalParsingExpression>(std::string{ *tok.currentToken() });
        tok.advance();
        return expr;
    } else if(tok.currentToken()->at(0) == '[') {
        // regex
        auto expr = std::make_shared<TerminalParsingExpression>(std::string{ *tok.currentToken() }, std::regex{ "^" + std::string{ *tok.currentToken() } });
        tok.advance();
        return expr;
    }
    throw std::runtime_error{ std::string{ "Invalid atomic token: '" } + std::string{ *tok.currentToken() } + "'" };
}

static sp<ParsingExpression> parseAttribute(ExpressionTokenizer& tok) {
    if(!tok.currentToken() || tok.currentToken()->empty()) {
        return nullptr;
    }
    if(*tok.currentToken() == "~sws~") {
        tok.advance();
        return std::make_shared<SkipWhitespacesExpression>(parseAtom(tok));
    }
    if(*tok.currentToken() == "~nws~") {
        tok.advance();
        return std::make_shared<DoNotSkipWhitespacesExpression>(parseAtom(tok));
    }
    if(*tok.currentToken() == "~fws~") {
        tok.advance();
        return std::make_shared<ForceSkippingWhitespacesExpression>(parseAtom(tok));
    }
    if(*tok.currentToken() == "~snn~") {
        tok.advance();
        return std::make_shared<SkipWhitespacesNoNewlinesExpression>(parseAtom(tok));
    }
    return std::make_shared<SkipWhitespacesExpression>(parseAtom(tok));
}

static sp<ParsingExpression> parsePrefix(ExpressionTokenizer& tok) {
    if(!tok.currentToken() || tok.currentToken()->empty()) {
        return nullptr;
    }
    if(*tok.currentToken() == "!") {
        tok.advance();
        return std::make_shared<NotParsingExpression>(parseAttribute(tok));
    }
    if(*tok.currentToken() == "&") {
        tok.advance();
        return std::make_shared<AndParsingExpression>(parseAttribute(tok));
    }
    return parseAttribute(tok);
}

static sp<ParsingExpression> parseQuantifier(ExpressionTokenizer& tok) {
    auto attrib = parsePrefix(tok);
    if(!tok.currentToken() || tok.currentToken()->empty()) {
        return attrib;
    }
    if(*tok.currentToken() == "?") {
        auto expr = std::make_shared<OptionalParsingExpression>(std::move(attrib));
        tok.advance();
        return expr;
    } else if(*tok.currentToken() == "*") {
        auto expr = std::make_shared<ZeroOrMoreParsingExpression>(std::move(attrib));
        tok.advance();
        return expr;
    } else if(*tok.currentToken() == "+") {
        auto expr = std::make_shared<OneOrMoreParsingExpression>(std::move(attrib));
        tok.advance();
        return expr;
    }
    return attrib;
}

static sp<ParsingExpression> parseErrorInfo(ExpressionTokenizer& tok) {
    auto quantifier = parseQuantifier(tok);
    if(!tok.currentToken() || tok.currentToken()->empty()) {
        return quantifier;
    }
    if(tok.currentToken()->at(0) == '#') {
        // error-info
        auto expr = std::make_shared<ErrorMessageInfoExpression>(std::move(quantifier), std::string{ tok.currentToken()->substr(1, tok.currentToken()->size() - 2) });
        tok.advance();
        return expr;
    }
    return quantifier;
}

static sp<ParsingExpression> parseSequence(ExpressionTokenizer& tok) {
    auto left = parseErrorInfo(tok);
    std::vector<sp<ParsingExpression>> children;
    children.emplace_back(std::move(left));
    while(tok.currentToken() && *tok.currentToken() != "|" && *tok.currentToken() != ")") {
        auto expr = parseErrorInfo(tok);
        if(!expr) {
            break;
        }
        children.emplace_back(expr);
    }
    if(children.size() == 1) {
        return std::move(children.at(0));
    }
    return std::make_shared<SequenceParsingExpression>(std::move(children));
}

static sp<ParsingExpression> parseChoice(ExpressionTokenizer& tok) {
    auto left = parseSequence(tok);
    std::vector<sp<ParsingExpression>> children;
    children.emplace_back(std::move(left));
    while(tok.currentToken() && *tok.currentToken() == "|") {
        tok.advance();
        children.emplace_back(parseSequence(tok));
    }
    if(children.size() == 1) {
        return std::move(children.at(0));
    }
    return std::make_shared<ChoiceParsingExpression>(std::move(children));
}

sp<ParsingExpression> stringToParsingExpression(const std::string_view& expr) {
    ExpressionTokenizer tok{ expr };
    return parseChoice(tok);
}

ExpressionTokenizer::ExpressionTokenizer(const std::string_view& expr)
: mExprString(expr) {
    genNextToken();
}

void ExpressionTokenizer::advance() {
    genNextToken();
}

void ExpressionTokenizer::genNextToken() {
    skipWhitespaces();
    auto start = mOffset;
    if(mOffset < mExprString.size()) {
        switch(getCurrentChar()) {
        case '\'':
            mCurrentToken = consumeString('\'', '\'');
            break;
        case '[':
            mCurrentToken = consumeString('[', ']');
            break;
        case '#':
            mCurrentToken = consumeString('#', '#');
            break;
        case '~':
            mCurrentToken = consumeString('~', '~');
            break;
        case '+':
        case '*':
        case ')':
        case '(':
        case '|':
        case '/':
        case '?':
        case '!':
        case '&':
            mOffset += 1;
            mCurrentToken = mExprString.substr(start, mOffset - start);
            break;
        default:
            if(isalnum(getCurrentChar())) {
                consumeNonTerminal();
                mCurrentToken = mExprString.substr(start, mOffset - start);
            } else {
                throw std::runtime_error{ std::string{ "Invalid input char: '" } + getCurrentChar() + "\'" };
            }
        }
    } else {
        mCurrentToken = {};
    }
}

void ExpressionTokenizer::consumeNonTerminal() {
    while(mOffset < mExprString.size() && (isalnum(getCurrentChar()) || getCurrentChar() == '_')) {
        mOffset += 1;
    }
}

std::string ExpressionTokenizer::consumeString(const char start, const char end) {
    assert(getCurrentChar() == start);
    std::string ret;
    ret += getCurrentChar();
    mOffset += 1;
    while(true) {
        if(mOffset >= mExprString.size()) {
            throw std::runtime_error{ "Unterminated string in expression!" };
        }
        if(getCurrentChar() == '\\') {
            if(getNextChar() == end || getNextChar() == '\\') {
                mOffset += 1;
                ret += getCurrentChar();
                mOffset += 1;
            }
        }
        if(getCurrentChar() == end) {
            ret += getCurrentChar();
            mOffset += 1;
            return ret;
        }
        ret += getCurrentChar();
        mOffset += 1;
    }
}

char ExpressionTokenizer::getCurrentChar() {
    return mExprString.at(mOffset);
}
char ExpressionTokenizer::getNextChar() {
    if(mExprString.size() <= mOffset + 1) {
        return -1;
    }
    return mExprString.at(mOffset + 1);
}

bool ExpressionTokenizer::skipWhitespaces() {
    const static std::string WHITESPACE_CHARS{ "\t \n" };
    bool didSkip = false;
    while(mExprString.size() > mOffset && WHITESPACE_CHARS.find(mExprString.at(mOffset)) != std::string::npos) {
        didSkip = true;
        mOffset += 1;
    }
    return didSkip;
}

}