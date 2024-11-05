#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <memory>
#include <unordered_map>

// Token Types
enum class TokenType {
    SELECT, INSERT, UPDATE, DELETE, FROM, WHERE, INTO, VALUES, SET, AND, OR, IDENTIFIER, NUMBER, COMMA, SEMICOLON, EQUALS, STAR, UNKNOWN
};

// Token representation
struct Token {
    TokenType type;
    std::string value;
};

// Exception class for SQL parsing errors
class SQLParseException : public std::runtime_error {
public:
    explicit SQLParseException(const std::string& message) : std::runtime_error(message) {}
};

// Abstract Syntax Tree (AST) Node base class
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void print() const = 0;
};

// ASTNode for SQL Identifier (table names, column names)
class ASTIdentifier : public ASTNode {
    std::string identifier;
public:
    explicit ASTIdentifier(const std::string& id) : identifier(id) {}
    void print() const override {
        std::cout << "Identifier(" << identifier << ")";
    }
};

// ASTNode for SQL Values (numbers, literals)
class ASTValue : public ASTNode {
    std::string value;
public:
    explicit ASTValue(const std::string& val) : value(val) {}
    void print() const override {
        std::cout << "Value(" << value << ")";
    }
};

// ASTNode for Binary Expressions (col = val)
class ASTBinaryExpression : public ASTNode {
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    std::string op;
public:
    ASTBinaryExpression(std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r, const std::string& o)
        : left(std::move(l)), right(std::move(r)), op(o) {}

    void print() const override {
        std::cout << "BinaryExpression(";
        left->print();
        std::cout << " " << op << " ";
        right->print();
        std::cout << ")";
    }
};

// ASTNode for SQL commands (SELECT, INSERT)
class ASTSQLCommand : public ASTNode {
    std::string command;
    std::vector<std::unique_ptr<ASTNode>> children;
public:
    explicit ASTSQLCommand(const std::string& cmd) : command(cmd) {}

    void addChild(std::unique_ptr<ASTNode> child) {
        children.push_back(std::move(child));
    }

    void print() const override {
        std::cout << "SQLCommand(" << command;
        for (const auto& child : children) {
            std::cout << ", ";
            child->print();
        }
        std::cout << ")";
    }
};

// Tokenizer class to split input SQL string into tokens
class Tokenizer {
    std::string input;
    size_t pos = 0;
    std::unordered_map<std::string, TokenType> keywords = {
        {"SELECT", TokenType::SELECT}, {"INSERT", TokenType::INSERT}, {"UPDATE", TokenType::UPDATE},
        {"DELETE", TokenType::DELETE}, {"FROM", TokenType::FROM}, {"WHERE", TokenType::WHERE},
        {"INTO", TokenType::INTO}, {"VALUES", TokenType::VALUES}, {"SET", TokenType::SET},
        {"AND", TokenType::AND}, {"OR", TokenType::OR}
    };

    bool isAlpha(char c) const { return std::isalpha(c); }
    bool isDigit(char c) const { return std::isdigit(c); }

public:
    explicit Tokenizer(const std::string& sql) : input(sql) {}

    Token nextToken() {
        while (pos < input.size() && std::isspace(input[pos])) ++pos;

        if (pos >= input.size()) return {TokenType::UNKNOWN, ""};

        char currentChar = input[pos];
        if (isAlpha(currentChar)) {
            size_t start = pos;
            while (pos < input.size() && (isAlpha(input[pos]) || isDigit(input[pos]))) ++pos;
            std::string keyword = input.substr(start, pos - start);
            if (keywords.count(keyword)) return {keywords[keyword], keyword};
            return {TokenType::IDENTIFIER, keyword};
        }

        if (isDigit(currentChar)) {
            size_t start = pos;
            while (pos < input.size() && isDigit(input[pos])) ++pos;
            return {TokenType::NUMBER, input.substr(start, pos - start)};
        }

        if (currentChar == ',') { ++pos; return {TokenType::COMMA, ","}; }
        if (currentChar == ';') { ++pos; return {TokenType::SEMICOLON, ";"}; }
        if (currentChar == '=') { ++pos; return {TokenType::EQUALS, "="}; }
        if (currentChar == '*') { ++pos; return {TokenType::STAR, "*"}; }

        ++pos;
        return {TokenType::UNKNOWN, std::string(1, currentChar)};
    }
};

// SQL Parser class to convert tokens into an AST
class SQLParser {
    Tokenizer tokenizer;
    Token currentToken;

    void consumeToken() { currentToken = tokenizer.nextToken(); }

    std::unique_ptr<ASTNode> parseIdentifier() {
        if (currentToken.type != TokenType::IDENTIFIER)
            throw SQLParseException("Expected identifier");
        auto node = std::make_unique<ASTIdentifier>(currentToken.value);
        consumeToken();
        return node;
    }

    std::unique_ptr<ASTNode> parseValue() {
        if (currentToken.type != TokenType::NUMBER)
            throw SQLParseException("Expected number");
        auto node = std::make_unique<ASTValue>(currentToken.value);
        consumeToken();
        return node;
    }

    std::unique_ptr<ASTNode> parseCondition() {
        auto left = parseIdentifier();
        if (currentToken.type != TokenType::EQUALS)
            throw SQLParseException("Expected '='");
        consumeToken();
        auto right = parseValue();
        return std::make_unique<ASTBinaryExpression>(std::move(left), std::move(right), "=");
    }

    std::unique_ptr<ASTNode> parseSelect() {
        consumeToken(); // consume 'SELECT'
        auto command = std::make_unique<ASTSQLCommand>("SELECT");
        command->addChild(parseIdentifier()); // column name
        consumeToken(); // consume 'FROM'
        command->addChild(parseIdentifier()); // table name
        if (currentToken.type == TokenType::WHERE) {
            consumeToken();
            command->addChild(parseCondition());
        }
        return command;
    }

public:
    explicit SQLParser(const std::string& sql) : tokenizer(sql) {
        consumeToken();
    }

    std::unique_ptr<ASTNode> parse() {
        if (currentToken.type == TokenType::SELECT) {
            return parseSelect();
        }
        throw SQLParseException("Unknown SQL command");
    }
};

int main() {
    try {
        SQLParser parser("SELECT column1 FROM table1 WHERE column1 = 10;");
        auto ast = parser.parse();
        ast->print();
    } catch (const SQLParseException& e) {
        std::cerr << "SQL Parse Error: " << e.what() << std::endl;
    }
    return 0;
}