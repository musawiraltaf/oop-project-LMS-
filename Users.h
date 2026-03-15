// ============================================================================
// Users.h — Role hierarchy with multi-level inheritance
// Chain: User → Junior → Employee → Manager → Director → Executive
// Demonstrates: 3+ levels of inheritance, virtual functions, polymorphism
// ============================================================================
#pragma once
#include<string>
#include"Utilities.h"
using namespace std;

// ======================== Base User Class ========================
class User {
protected:
    string username, passhash, passsalt;
    int    clearanceLevel;
public:
    User(const string &u, const string &h, const string &s, int lvl);
    virtual ~User();

    // Authentication
    bool authenticate(const string &pwd) const;

    // Accessors
    const string& getUsername() const;
    int getClearance() const;

    // --- Virtual functions for runtime polymorphism ---
    virtual string getRoleTitle() const;
    virtual void   displayDashboard() const;
    virtual bool   canPerformAction(const string &action) const;

    // --- Operator overloads ---
    bool operator<(const User &o) const;
    bool operator==(const User &o) const;
};

// ============================================================================
// Multi-level inheritance chain (3+ hierarchical levels)
//   Level 1: Junior    (inherits User)
//   Level 2: Employee  (inherits Junior)
//   Level 3: Manager   (inherits Employee)
//   Level 4: Director  (inherits Manager)
//   Level 5: Executive (inherits Director)
// ============================================================================

class Junior : public User {
protected:
    // Protected constructor for subclass chaining
    Junior(const string &u, const string &h, const string &s, int lvl);
public:
    Junior(const string &u, const string &h, const string &s);
    string getRoleTitle() const override;
    void   displayDashboard() const override;
    bool   canPerformAction(const string &action) const override;
};

class Employee : public Junior {
protected:
    Employee(const string &u, const string &h, const string &s, int lvl);
public:
    Employee(const string &u, const string &h, const string &s);
    string getRoleTitle() const override;
    void   displayDashboard() const override;
    bool   canPerformAction(const string &action) const override;
};

class Manager : public Employee {
protected:
    Manager(const string &u, const string &h, const string &s, int lvl);
public:
    Manager(const string &u, const string &h, const string &s);
    string getRoleTitle() const override;
    void   displayDashboard() const override;
    bool   canPerformAction(const string &action) const override;
};

class Director : public Manager {
protected:
    Director(const string &u, const string &h, const string &s, int lvl);
public:
    Director(const string &u, const string &h, const string &s);
    string getRoleTitle() const override;
    void   displayDashboard() const override;
    bool   canPerformAction(const string &action) const override;
};

class Executive : public Director {
public:
    Executive(const string &u, const string &h, const string &s);
    string getRoleTitle() const override;
    void   displayDashboard() const override;
    bool   canPerformAction(const string &action) const override;
};
