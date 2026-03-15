// ============================================================================
// Utilities.h — Core utility classes for the OSIM system
// Contains: Color codes, Clock, PasswordManager, AuditLogger, MFAHandler,
//           CredentialManager, PolicyEngine, AnomalyDetector, GlobalNotifier,
//           PerformanceReport, and helper functions.
// ============================================================================
#pragma once
#include<string>
#include<ctime>
#include<fstream>
#include<iostream>
using namespace std;

// ======================== ANSI Color Codes ========================
namespace Color {
    const char* const RESET   = "\033[0m";
    const char* const BOLD    = "\033[1m";
    const char* const DIM     = "\033[2m";
    const char* const RED     = "\033[31m";
    const char* const GREEN   = "\033[32m";
    const char* const YELLOW  = "\033[33m";
    const char* const BLUE    = "\033[34m";
    const char* const MAGENTA = "\033[35m";
    const char* const CYAN    = "\033[36m";
    const char* const WHITE   = "\033[37m";
    const char* const BRED    = "\033[91m";
    const char* const BGREEN  = "\033[92m";
    const char* const BYELLOW = "\033[93m";
    const char* const BBLUE   = "\033[94m";
    const char* const BCYAN   = "\033[96m";
    const char* const BG_DARK = "\033[48;5;235m";
}

// ======================== Clearance Constants ========================
const int CLEARANCE_JUNIOR    = 1;
const int CLEARANCE_EMPLOYEE  = 2;
const int CLEARANCE_MANAGER   = 3;
const int CLEARANCE_DIRECTOR  = 4;
const int CLEARANCE_EXECUTIVE = 5;

// ======================== Time Alias ========================
typedef time_t Time;

// ======================== Clock ========================
class Clock {
public:
    static Time now();
};

// ======================== PasswordManager ========================
class PasswordManager {
public:
    static string gensalt(const string &password);
    static string hashpass(const string &password, const string &salt);
};

// ======================== AuditEntry (for operator<< logging) ========================
struct AuditEntry {
    string username;
    string action;
    string detail;
};

// ======================== AuditLogger ========================
class AuditLogger {
    const string filename = "audit.txt";
public:
    void log(const string &username, const string &action, const string &detail);
    void logFailure(const string &username);
    // Operator << overload for structured logging (spec requirement)
    friend AuditLogger& operator<<(AuditLogger &logger, const AuditEntry &entry);
    const string& getFilename() const;
};

// ======================== MFAHandler ========================
class MFAHandler {
    string currentOTP;
    Time   sentAt = 0;
    const int Ttl = 300;
    const char* otpFile = "otp.txt";
public:
    void sendOTP(const string &user);
    bool validateOTP(const string &in);
};

// ======================== CredentialManager ========================
class User;  // forward declaration

class CredentialManager {
    static const int MAX_USERS = 100;
    User* users[MAX_USERS];
    int   userCount;

    // --- Login lockout tracking ---
    static const int MAX_ATTEMPTS = 3;
    struct LoginAttempt {
        string username;
        int failCount;
    };
    LoginAttempt attempts[MAX_USERS];
    int attemptCount;

    void clearAll();
public:
    CredentialManager();
    ~CredentialManager();

    void  load(const char* filename = "users.txt");
    User* authenticate(const string &uname, const string &pwd) const;
    User** getUsers(int &outCount) const;
    bool  addUser(const string &uname, const string &hash, const string &salt,
                  const string &role, const char* filename = "users.txt");

    // Login lockout methods
    bool isLocked(const string &username) const;
    void recordFailure(const string &username);
    void resetAttempts(const string &username);
};

// ======================== PolicyEngine ========================
class PolicyEngine {
public:
    // --- Core permission checks (centralized, no hardcoded checks) ---
    static bool canCreate(const User* creator, const User* target);
    static bool canDelegate(const User* sender, const User* newAssignee);
    static bool canSend(const User* sender, const User* recipient, int typeCode);
    static bool canAddUser(const User* adder, int targetRole);
    static bool canViewAllReports(const User* viewer);
    static bool canViewAnomalies(const User* viewer);
    static bool canClearAnomalies(const User* viewer);
    static bool canSendNotification(const User* sender, int level);

    // Functor operator() — general permission check (spec requirement)
    bool operator()(const User* user, const string &action) const;
};

// ======================== GlobalLevel Enum ========================
enum GlobalLevel { INFO_SYS, WARNING_SYS, EMERGENCY_SYS };

// ======================== AnomalyDetector ========================
class MessageManager; // forward declaration

class AnomalyDetector {
    bool skipNextAlert = false;
    bool reported      = false;
    Time lastClearTime = 0;
    AuditLogger &logger;
    MessageManager &msgs;
    const int MAX_LOGIN_ERRORS = 2;
    const int MAX_REASSIGN     = 2;
public:
    AnomalyDetector(AuditLogger &logRef, MessageManager &msgRef);
    void clearAnomalies();
    void scanAndReport(User* invoker, User** users, int userCount);
    // operator() — functor to scan anomalies (spec requirement)
    bool operator()(User* invoker, User** users, int userCount);
};

// ======================== GlobalNotifier ========================
class GlobalNotifier {
    AuditLogger &logger;
    MessageManager &msgs;
public:
    GlobalNotifier(AuditLogger &logRef, MessageManager &msgRef);
    void send(User* sender, GlobalLevel lvl, const string &text,
              User** users, int userCount);
};

// ======================== PerformanceReport ========================
struct PerformanceReport {
    int total  = 0;
    int onTime = 0;
    int late   = 0;
    // operator<< as friend (spec requirement)
    friend ostream& operator<<(ostream &os, const PerformanceReport &rpt);
    // operator+= to accumulate stats (spec requirement)
    PerformanceReport& operator+=(const PerformanceReport &other);
};

PerformanceReport readReport(const string &user);
void updateReport(const char empID[], bool onTime);

// ======================== UI & Input Helpers ========================
void handleLogin(User*& current, CredentialManager &creds, MFAHandler &mfa,
                 AuditLogger &logger, AnomalyDetector &anom);
void printBoxTitle(const string &title);
void printColorLine(const string &text, const char* color);
void printSuccess(const string &text);
void printError(const string &text);
void printWarning(const string &text);
void printInfo(const string &text);
int  inputInt(const string &prompt);
string inputString(const string &prompt);
