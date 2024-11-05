#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>

class Query {
public:
    std::string operation;
    std::string collection;
    std::map<std::string, std::string> fields;
    std::map<std::string, std::string> conditions;

    Query(std::string op, std::string coll) : operation(op), collection(coll) {}

    void addField(const std::string& key, const std::string& value) {
        fields[key] = value;
    }

    void addCondition(const std::string& key, const std::string& value) {
        conditions[key] = value;
    }

    void print() {
        std::cout << "Operation: " << operation << std::endl;
        std::cout << "Collection: " << collection << std::endl;
        std::cout << "Fields: ";
        for (const auto& field : fields) {
            std::cout << field.first << ": " << field.second << ", ";
        }
        std::cout << std::endl;
        std::cout << "Conditions: ";
        for (const auto& condition : conditions) {
            std::cout << condition.first << ": " << condition.second << ", ";
        }
        std::cout << std::endl;
    }
};

class NoSQLParser {
public:
    Query parse(const std::string& query) {
        std::istringstream stream(query);
        std::string token;
        std::vector<std::string> tokens;

        while (stream >> token) {
            tokens.push_back(token);
        }

        std::string operation = tokens[0];
        std::string collection = tokens[1];

        Query parsedQuery(operation, collection);

        if (operation == "INSERT") {
            parseInsert(tokens, parsedQuery);
        } else if (operation == "SELECT") {
            parseSelect(tokens, parsedQuery);
        } else if (operation == "UPDATE") {
            parseUpdate(tokens, parsedQuery);
        } else if (operation == "DELETE") {
            parseDelete(tokens, parsedQuery);
        } else {
            std::cerr << "Unknown operation: " << operation << std::endl;
        }

        return parsedQuery;
    }

private:
    void parseInsert(const std::vector<std::string>& tokens, Query& query) {
        size_t start = 3;  // Starts after 'INSERT INTO collection'
        for (size_t i = start; i < tokens.size(); i += 2) {
            query.addField(tokens[i], tokens[i + 1]);
        }
    }

    void parseSelect(const std::vector<std::string>& tokens, Query& query) {
        size_t wherePos = findWherePosition(tokens);

        for (size_t i = 2; i < wherePos; ++i) {
            query.addField(tokens[i], "");
        }

        if (wherePos < tokens.size()) {
            parseWhereClause(tokens, wherePos, query);
        }
    }

    void parseUpdate(const std::vector<std::string>& tokens, Query& query) {
        size_t setPos = findPosition(tokens, "SET");
        size_t wherePos = findPosition(tokens, "WHERE");

        for (size_t i = setPos + 1; i < wherePos; i += 2) {
            query.addField(tokens[i], tokens[i + 1]);
        }

        if (wherePos < tokens.size()) {
            parseWhereClause(tokens, wherePos, query);
        }
    }

    void parseDelete(const std::vector<std::string>& tokens, Query& query) {
        size_t wherePos = findPosition(tokens, "WHERE");

        if (wherePos < tokens.size()) {
            parseWhereClause(tokens, wherePos, query);
        }
    }

    size_t findPosition(const std::vector<std::string>& tokens, const std::string& keyword) {
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i] == keyword) {
                return i;
            }
        }
        return tokens.size();
    }

    size_t findWherePosition(const std::vector<std::string>& tokens) {
        return findPosition(tokens, "WHERE");
    }

    void parseWhereClause(const std::vector<std::string>& tokens, size_t wherePos, Query& query) {
        for (size_t i = wherePos + 1; i < tokens.size(); i += 2) {
            query.addCondition(tokens[i], tokens[i + 1]);
        }
    }
};

int main() {
    NoSQLParser parser;

    std::string query1 = "INSERT INTO users name Mike age 30";
    Query result1 = parser.parse(query1);
    result1.print();

    std::string query2 = "SELECT name age FROM users WHERE id 123";
    Query result2 = parser.parse(query2);
    result2.print();

    std::string query3 = "UPDATE users SET name Mike age 31 WHERE id 123";
    Query result3 = parser.parse(query3);
    result3.print();

    std::string query4 = "DELETE FROM users WHERE id 123";
    Query result4 = parser.parse(query4);
    result4.print();

    return 0;
}