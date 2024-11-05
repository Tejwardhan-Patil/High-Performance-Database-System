#include <iostream>
#include <cassert>
#include "security/authentication/auth_server.h"
#include "security/authorization/rbac.h"
#include "security/encryption/encryption_at_rest.h"
#include "security/encryption/encryption_in_transit.h"
#include "security/audit_logs.h"

void test_authentication() {
    AuthServer authServer;

    // Test valid credentials
    std::string token = authServer.authenticate("valid_user", "valid_password");
    assert(!token.empty() && "Authentication failed for valid user");

    // Test invalid credentials
    try {
        authServer.authenticate("invalid_user", "invalid_password");
        assert(false && "Authentication passed for invalid user");
    } catch (const std::exception &e) {
        std::cout << "Authentication failed as expected: " << e.what() << std::endl;
    }
}

void test_role_based_access_control() {
    RBAC rbac;

    // Test assigning roles
    rbac.assignRole("admin", "manage_users");
    assert(rbac.hasPermission("admin", "manage_users") && "Admin role should have manage_users permission");

    // Test revoking roles
    rbac.revokeRole("admin", "manage_users");
    assert(!rbac.hasPermission("admin", "manage_users") && "Admin role should not have manage_users permission after revoking");

    // Test unauthorized access
    try {
        assert(rbac.hasPermission("user", "manage_users") == false && "User should not have manage_users permission");
    } catch (const std::exception &e) {
        std::cout << "Access denied as expected: " << e.what() << std::endl;
    }
}

void test_encryption_at_rest() {
    EncryptionAtRest encryption;

    // Test encryption and decryption of data
    std::string plaintext = "sensitive_data";
    std::string ciphertext = encryption.encrypt(plaintext);
    assert(!ciphertext.empty() && "Encryption failed");

    std::string decryptedText = encryption.decrypt(ciphertext);
    assert(decryptedText == plaintext && "Decryption failed");
}

void test_encryption_in_transit() {
    EncryptionInTransit encryption;

    // Simulate data transmission with encryption
    std::string data = "sensitive_data";
    std::string encryptedData = encryption.encrypt(data);
    assert(!encryptedData.empty() && "Encryption in transit failed");

    // Simulate data reception and decryption
    std::string decryptedData = encryption.decrypt(encryptedData);
    assert(decryptedData == data && "Decryption in transit failed");
}

void test_audit_logs() {
    AuditLogs auditLogs;

    // Log a successful operation
    auditLogs.log("User logged in", "valid_user", "SUCCESS");

    // Log a failed operation
    auditLogs.log("User login failed", "invalid_user", "FAILURE");

    // Verify logs
    auto logs = auditLogs.getLogs();
    assert(logs.size() == 2 && "Audit logs not working as expected");
    assert(logs[0].status == "SUCCESS" && logs[1].status == "FAILURE" && "Log status incorrect");
}

void test_multiple_auth_failures() {
    AuthServer authServer;
    for (int i = 0; i < 5; i++) {
        try {
            authServer.authenticate("invalid_user", "wrong_password");
        } catch (const std::exception &) {
            std::cout << "Login attempt " << i + 1 << " failed" << std::endl;
        }
    }
    // Ensure lockout mechanism triggers after multiple failures
    try {
        authServer.authenticate("invalid_user", "wrong_password");
        assert(false && "Lockout did not trigger after multiple failed attempts");
    } catch (const std::exception &e) {
        std::cout << "Account locked after multiple failed attempts: " << e.what() << std::endl;
    }
}

void test_password_encryption() {
    AuthServer authServer;

    // Verify password encryption on signup
    std::string password = "my_secure_password";
    std::string encryptedPassword = authServer.encryptPassword(password);
    assert(!encryptedPassword.empty() && "Password encryption failed");

    // Verify password matching during authentication
    std::string storedPassword = encryptedPassword;
    bool isPasswordCorrect = authServer.verifyPassword(storedPassword, password);
    assert(isPasswordCorrect && "Password verification failed");
}

void test_firewall_rules() {
    // Mock firewall test cases for unauthorized access
    Firewall firewall;

    // Block unauthorized IP
    firewall.blockIP("192.168.0.1");
    bool isBlocked = firewall.isBlocked("192.168.0.1");
    assert(isBlocked && "Firewall rule to block IP failed");

    // Allow authorized IP
    firewall.allowIP("192.168.0.2");
    bool isAllowed = firewall.isAllowed("192.168.0.2");
    assert(isAllowed && "Firewall rule to allow IP failed");
}

void test_oauth2_authentication() {
    OAuth2 oauth2;

    // Test OAuth2 token issuance
    std::string token = oauth2.issueToken("user_id");
    assert(!token.empty() && "OAuth2 token issuance failed");

    // Test OAuth2 token validation
    bool isValid = oauth2.validateToken(token);
    assert(isValid && "OAuth2 token validation failed");

    // Test invalid token rejection
    isValid = oauth2.validateToken("invalid_token");
    assert(!isValid && "Invalid OAuth2 token should not be validated");
}

void test_security_flaws_detection() {
    // Simulate an attack vector detection
    SecurityMonitor securityMonitor;
    securityMonitor.detectSQLInjection("SELECT * FROM users WHERE id = 1; DROP TABLE users;");
    bool isSQLInjection = securityMonitor.isSQLInjectionDetected();
    assert(isSQLInjection && "Failed to detect SQL injection");

    // Simulate XSS detection
    securityMonitor.detectXSS("<script>alert('XSS');</script>");
    bool isXSSDetected = securityMonitor.isXSSDetected();
    assert(isXSSDetected && "Failed to detect XSS attack");
}

int main() {
    std::cout << "Running security tests..." << std::endl;

    test_authentication();
    test_role_based_access_control();
    test_encryption_at_rest();
    test_encryption_in_transit();
    test_audit_logs();
    test_multiple_auth_failures();
    test_password_encryption();
    test_firewall_rules();
    test_oauth2_authentication();
    test_security_flaws_detection();

    std::cout << "All security tests passed!" << std::endl;
    return 0;
}