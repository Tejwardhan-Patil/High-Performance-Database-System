#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

class RBAC {
public:
    // Define roles and permissions
    enum class Permission { READ, WRITE, DELETE, EXECUTE };
    struct Role {
        std::string name;
        std::unordered_set<Permission> permissions;

        Role(const std::string& roleName) : name(roleName) {}

        void addPermission(Permission permission) {
            permissions.insert(permission);
        }

        bool hasPermission(Permission permission) const {
            return permissions.find(permission) != permissions.end();
        }
    };

    // User class representing a user with assigned roles
    struct User {
        std::string username;
        std::unordered_set<std::string> roles;

        User(const std::string& name) : username(name) {}

        void assignRole(const std::string& role) {
            roles.insert(role);
        }

        bool hasRole(const std::string& role) const {
            return roles.find(role) != roles.end();
        }
    };

    // Role-based access control system
    class AccessControlSystem {
    private:
        std::unordered_map<std::string, Role> roleMap;
        std::unordered_map<std::string, User> userMap;

    public:
        void createRole(const std::string& roleName) {
            roleMap[roleName] = Role(roleName);
        }

        void addUser(const std::string& username) {
            userMap[username] = User(username);
        }

        void assignRoleToUser(const std::string& username, const std::string& roleName) {
            if (userMap.find(username) != userMap.end() && roleMap.find(roleName) != roleMap.end()) {
                userMap[username].assignRole(roleName);
            }
        }

        void addPermissionToRole(const std::string& roleName, Permission permission) {
            if (roleMap.find(roleName) != roleMap.end()) {
                roleMap[roleName].addPermission(permission);
            }
        }

        bool checkUserPermission(const std::string& username, Permission permission) {
            if (userMap.find(username) != userMap.end()) {
                for (const auto& role : userMap[username].roles) {
                    if (roleMap[role].hasPermission(permission)) {
                        return true;
                    }
                }
            }
            return false;
        }

        void removeUser(const std::string& username) {
            userMap.erase(username);
        }

        void removeRole(const std::string& roleName) {
            roleMap.erase(roleName);
        }
    };
};

std::ostream& operator<<(std::ostream& os, const RBAC::Permission& permission) {
    switch (permission) {
        case RBAC::Permission::READ: os << "READ"; break;
        case RBAC::Permission::WRITE: os << "WRITE"; break;
        case RBAC::Permission::DELETE: os << "DELETE"; break;
        case RBAC::Permission::EXECUTE: os << "EXECUTE"; break;
    }
    return os;
}

// Usage of the RBAC system
int main() {
    RBAC::AccessControlSystem acs;

    // Create roles
    acs.createRole("Admin");
    acs.createRole("Editor");
    acs.createRole("Viewer");

    // Add permissions to roles
    acs.addPermissionToRole("Admin", RBAC::Permission::READ);
    acs.addPermissionToRole("Admin", RBAC::Permission::WRITE);
    acs.addPermissionToRole("Admin", RBAC::Permission::DELETE);
    acs.addPermissionToRole("Editor", RBAC::Permission::READ);
    acs.addPermissionToRole("Editor", RBAC::Permission::WRITE);
    acs.addPermissionToRole("Viewer", RBAC::Permission::READ);

    // Add users
    acs.addUser("Person1");
    acs.addUser("Person2");
    acs.addUser("Person3");

    // Assign roles to users
    acs.assignRoleToUser("Person1", "Admin");
    acs.assignRoleToUser("Person2", "Editor");
    acs.assignRoleToUser("Person3", "Viewer");

    // Check permissions for users
    std::cout << "Does Person1 have WRITE permission? "
              << (acs.checkUserPermission("Person1", RBAC::Permission::WRITE) ? "Yes" : "No") << std::endl;
    std::cout << "Does Person2 have DELETE permission? "
              << (acs.checkUserPermission("Person2", RBAC::Permission::DELETE) ? "Yes" : "No") << std::endl;
    std::cout << "Does Person3 have READ permission? "
              << (acs.checkUserPermission("Person3", RBAC::Permission::READ) ? "Yes" : "No") << std::endl;

    // Remove a user
    acs.removeUser("Person3");
    std::cout << "Does Person3 have READ permission after removal? "
              << (acs.checkUserPermission("Person3", RBAC::Permission::READ) ? "Yes" : "No") << std::endl;

    return 0;
}