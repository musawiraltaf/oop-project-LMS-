// ============================================================================
// Users.cpp — Implementation of multi-level role hierarchy
// Chain: User → Junior → Employee → Manager → Director → Executive
// Each level adds unique behavior via virtual function overrides
// ============================================================================
#include"Users.h"
#include<iostream>
using namespace std;

// ======================== User (Base) ========================
User::User(const string &u, const string &h, const string &s, int lvl)
    : username(u), passhash(h), passsalt(s), clearanceLevel(lvl) {}

User::~User() {}

bool User::authenticate(const string &pwd) const {
    return PasswordManager::hashpass(pwd, passsalt) == passhash;
}

const string& User::getUsername() const { return username; }
int User::getClearance() const { return clearanceLevel; }

bool User::operator<(const User &o) const { return username < o.username; }
bool User::operator==(const User &o) const { return username == o.username; }

string User::getRoleTitle() const { return "User"; }

void User::displayDashboard() const {
    cout << Color::CYAN << "  Role: " << Color::BOLD << getRoleTitle()
         << Color::RESET << Color::CYAN
         << " | Clearance Level: " << clearanceLevel << Color::RESET << "\n";
}

bool User::canPerformAction(const string &action) const {
    return false; // base class denies all actions by default
}

// ======================== Junior (Level 1 — inherits User) ========================
Junior::Junior(const string &u, const string &h, const string &s)
    : User(u, h, s, CLEARANCE_JUNIOR) {}

Junior::Junior(const string &u, const string &h, const string &s, int lvl)
    : User(u, h, s, lvl) {}

string Junior::getRoleTitle() const { return "Junior"; }

void Junior::displayDashboard() const {
    User::displayDashboard();
    cout << Color::DIM << "  Permissions: View tasks, Send INFO/PRIVATE messages\n"
         << Color::RESET;
}

bool Junior::canPerformAction(const string &action) const {
    if (action == "VIEW_TASKS")   return true;
    if (action == "SEND_INFO")    return true;
    if (action == "SEND_PRIVATE") return true;
    if (action == "COMPLETE_TASK") return true;
    if (action == "START_TASK")   return true;
    return false;
}

// ======================== Employee (Level 2 — inherits Junior) ========================
Employee::Employee(const string &u, const string &h, const string &s)
    : Junior(u, h, s, CLEARANCE_EMPLOYEE) {}

Employee::Employee(const string &u, const string &h, const string &s, int lvl)
    : Junior(u, h, s, lvl) {}

string Employee::getRoleTitle() const { return "Employee"; }

void Employee::displayDashboard() const {
    User::displayDashboard();
    cout << Color::DIM
         << "  Permissions: All Junior perms + Delegate tasks\n"
         << Color::RESET;
}

bool Employee::canPerformAction(const string &action) const {
    // Inherits all Junior permissions via parent call
    if (Junior::canPerformAction(action)) return true;
    if (action == "DELEGATE_TASK") return true;
    return false;
}

// ======================== Manager (Level 3 — inherits Employee) ========================
Manager::Manager(const string &u, const string &h, const string &s)
    : Employee(u, h, s, CLEARANCE_MANAGER) {}

Manager::Manager(const string &u, const string &h, const string &s, int lvl)
    : Employee(u, h, s, lvl) {}

string Manager::getRoleTitle() const { return "Manager"; }

void Manager::displayDashboard() const {
    User::displayDashboard();
    cout << Color::DIM
         << "  Permissions: All Employee perms + Add users, View anomalies,"
         << " Send WARNING\n" << Color::RESET;
}

bool Manager::canPerformAction(const string &action) const {
    if (Employee::canPerformAction(action)) return true;
    if (action == "ADD_USER")         return true;
    if (action == "VIEW_ANOMALIES")   return true;
    if (action == "CLEAR_ANOMALIES")  return true;
    if (action == "CREATE_TASK")      return true;
    if (action == "SEND_WARNING")     return true;
    return false;
}

// ======================== Director (Level 4 — inherits Manager) ========================
Director::Director(const string &u, const string &h, const string &s)
    : Manager(u, h, s, CLEARANCE_DIRECTOR) {}

Director::Director(const string &u, const string &h, const string &s, int lvl)
    : Manager(u, h, s, lvl) {}

string Director::getRoleTitle() const { return "Director"; }

void Director::displayDashboard() const {
    User::displayDashboard();
    cout << Color::DIM
         << "  Permissions: All Manager perms + View all reports, Send ALERT\n"
         << Color::RESET;
}

bool Director::canPerformAction(const string &action) const {
    if (Manager::canPerformAction(action)) return true;
    if (action == "VIEW_ALL_REPORTS") return true;
    if (action == "SEND_ALERT")       return true;
    return false;
}

// ======================== Executive (Level 5 — inherits Director) ========================
Executive::Executive(const string &u, const string &h, const string &s)
    : Director(u, h, s, CLEARANCE_EXECUTIVE) {}

string Executive::getRoleTitle() const { return "Executive"; }

void Executive::displayDashboard() const {
    User::displayDashboard();
    cout << Color::DIM
         << "  Permissions: Full access — all features unlocked\n"
         << Color::RESET;
}

bool Executive::canPerformAction(const string &action) const {
    // Executive can perform ALL actions
    return true;
}
