#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "encryption_at_rest.h"
#include "rbac.h"
#include "auth_server.h"
#include "audit_logs.h"

class SecurityTest {
public:
    void runTests() {
        testAuthentication();
        testAuthorization();
        testEncryptionAtRest();
        testAuditLogging();
        testAccessControl();
        testEncryptionInTransit();
        std::cout << "All security tests passed!" << std::endl;
    }

private:
    void testAuthentication() {
        AuthServer authServer;
        std::string validUsername = "person";
        std::string validPassword = "strongpassword";

        // Test valid login
        assert(authServer.authenticate(validUsername, validPassword) == true);

        // Test invalid login
        std::string invalidUsername = "invalid";
        std::string invalidPassword = "weakpassword";
        assert(authServer.authenticate(invalidUsername, invalidPassword) == false);

        std::cout << "Authentication tests passed!" << std::endl;
    }

    void testAuthorization() {
        RBAC rbacSystem;
        rbacSystem.addRole("admin");
        rbacSystem.addRole("user");

        rbacSystem.addPermissionToRole("admin", "read");
        rbacSystem.addPermissionToRole("admin", "write");
        rbacSystem.addPermissionToRole("user", "read");

        rbacSystem.assignRoleToUser("person", "admin");

        // Test permissions for admin
        assert(rbacSystem.userHasPermission("person", "read") == true);
        assert(rbacSystem.userHasPermission("person", "write") == true);

        // Test permissions for a normal user
        rbacSystem.assignRoleToUser("person2", "user");
        assert(rbacSystem.userHasPermission("person2", "read") == true);
        assert(rbacSystem.userHasPermission("person2", "write") == false);

        std::cout << "Authorization tests passed!" << std::endl;
    }

    void testEncryptionAtRest() {
        std::string data = "Sensitive data to be encrypted.";
        EncryptionAtRest encryption;

        std::string encryptedData = encryption.encrypt(data);
        std::string decryptedData = encryption.decrypt(encryptedData);

        assert(data == decryptedData);
        std::cout << "Encryption-at-rest tests passed!" << std::endl;
    }

    void testAuditLogging() {
        AuditLogs audit;
        audit.logAccess("person", "read", "database");
        audit.logAccess("person2", "write", "log_file");

        std::vector<std::string> logs = audit.getLogs();
        assert(logs.size() == 2);

        assert(logs[0].find("person") != std::string::npos);
        assert(logs[1].find("person2") != std::string::npos);

        std::cout << "Audit log tests passed!" << std::endl;
    }

    void testAccessControl() {
        RBAC rbacSystem;
        rbacSystem.addRole("editor");
        rbacSystem.addPermissionToRole("editor", "edit");

        rbacSystem.assignRoleToUser("person3", "editor");

        // Test permission assignment
        assert(rbacSystem.userHasPermission("person3", "edit") == true);
        assert(rbacSystem.userHasPermission("person3", "delete") == false);

        std::cout << "Access control tests passed!" << std::endl;
    }

    void testEncryptionInTransit() {
        std::string message = "Secure communication message.";
        EncryptionInTransit encryption;

        std::string encryptedMessage = encryption.encrypt(message);
        std::string decryptedMessage = encryption.decrypt(encryptedMessage);

        assert(message == decryptedMessage);
        std::cout << "Encryption-in-transit tests passed!" << std::endl;
    }
};

int main() {
    SecurityTest securityTest;
    securityTest.runTests();
    return 0;
}

// Implementation of mock classes for the purposes of the test

// EncryptionAtRest.h
class EncryptionAtRest {
public:
    std::string encrypt(const std::string &data) {
        return "encrypted_" + data;
    }

    std::string decrypt(const std::string &encryptedData) {
        return encryptedData.substr(10);  // Removing "encrypted_"
    }
};

// RBAC.h
class RBAC {
private:
    std::map<std::string, std::vector<std::string>> rolePermissions;
    std::map<std::string, std::string> userRoles;

public:
    void addRole(const std::string &role) {
        rolePermissions[role] = std::vector<std::string>();
    }

    void addPermissionToRole(const std::string &role, const std::string &permission) {
        rolePermissions[role].push_back(permission);
    }

    void assignRoleToUser(const std::string &user, const std::string &role) {
        userRoles[user] = role;
    }

    bool userHasPermission(const std::string &user, const std::string &permission) {
        std::string role = userRoles[user];
        return std::find(rolePermissions[role].begin(), rolePermissions[role].end(), permission) != rolePermissions[role].end();
    }
};

// AuthServer.h
class AuthServer {
public:
    bool authenticate(const std::string &username, const std::string &password) {
        if (username == "person" && password == "strongpassword") {
            return true;
        }
        return false;
    }
};

// AuditLogs.h
class AuditLogs {
private:
    std::vector<std::string> logs;

public:
    void logAccess(const std::string &user, const std::string &action, const std::string &resource) {
        logs.push_back(user + " performed " + action + " on " + resource);
    }

    std::vector<std::string> getLogs() {
        return logs;
    }
};

// EncryptionInTransit.h
class EncryptionInTransit {
public:
    std::string encrypt(const std::string &data) {
        return "encrypted_transit_" + data;
    }

    std::string decrypt(const std::string &encryptedData) {
        return encryptedData.substr(17);  // Removing "encrypted_transit_"
    }
};