#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <unordered_map>
#include <mutex>

class AuditLogger {
private:
    std::ofstream logFile;
    std::mutex logMutex;

    std::string getCurrentTime() {
        time_t now = time(0);
        tm *ltm = localtime(&now);
        char buffer[20];
        sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", 1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
        return std::string(buffer);
    }

public:
    AuditLogger(const std::string &filename) {
        logFile.open(filename, std::ios::out | std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "Error: Could not open log file: " << filename << std::endl;
            exit(1);
        }
    }

    ~AuditLogger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    void logEvent(const std::string &event, const std::string &user, const std::string &details) {
        std::lock_guard<std::mutex> guard(logMutex);
        logFile << "[" << getCurrentTime() << "] "
                << "User: " << user << " | Event: " << event
                << " | Details: " << details << std::endl;
    }
};

class AuthenticationSystem {
private:
    AuditLogger &auditLogger;
    std::unordered_map<std::string, std::string> userCredentials;

public:
    AuthenticationSystem(AuditLogger &logger) : auditLogger(logger) {
        // Initializing with some users
        userCredentials["admin"] = "admin_password";
        userCredentials["user1"] = "password1";
        userCredentials["user2"] = "password2";
    }

    bool authenticate(const std::string &username, const std::string &password) {
        if (userCredentials.find(username) != userCredentials.end() && userCredentials[username] == password) {
            auditLogger.logEvent("LoginSuccess", username, "User successfully logged in.");
            return true;
        } else {
            auditLogger.logEvent("LoginFailed", username, "Failed login attempt.");
            return false;
        }
    }
};

class DatabaseAccessControl {
private:
    AuditLogger &auditLogger;

public:
    DatabaseAccessControl(AuditLogger &logger) : auditLogger(logger) {}

    void accessData(const std::string &username, const std::string &data) {
        auditLogger.logEvent("DataAccess", username, "Accessed data: " + data);
    }

    void modifyData(const std::string &username, const std::string &data, const std::string &modification) {
        auditLogger.logEvent("DataModification", username, "Modified data: " + data + " | Modification: " + modification);
    }

    void deleteData(const std::string &username, const std::string &data) {
        auditLogger.logEvent("DataDeletion", username, "Deleted data: " + data);
    }
};

class ErrorHandling {
private:
    AuditLogger &auditLogger;

public:
    ErrorHandling(AuditLogger &logger) : auditLogger(logger) {}

    void logError(const std::string &username, const std::string &errorMessage) {
        auditLogger.logEvent("Error", username, "Error message: " + errorMessage);
    }
};

// Usage 
int main() {
    AuditLogger auditLogger("audit_log.txt");

    AuthenticationSystem authSystem(auditLogger);
    DatabaseAccessControl dbAccess(auditLogger);
    ErrorHandling errorHandling(auditLogger);

    // Login attempts
    if (authSystem.authenticate("admin", "admin_password")) {
        dbAccess.accessData("admin", "SensitiveData");
        dbAccess.modifyData("admin", "SensitiveData", "UpdatedValue");
        dbAccess.deleteData("admin", "ObsoleteData");
    }

    if (!authSystem.authenticate("user1", "wrong_password")) {
        errorHandling.logError("user1", "Invalid login attempt.");
    }

    if (authSystem.authenticate("user1", "password1")) {
        dbAccess.accessData("user1", "GeneralData");
    }

    return 0;
}