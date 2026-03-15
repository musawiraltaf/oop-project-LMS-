// ============================================================================
// Utilities.cpp — Implementation of all core utility classes
// Contains: Clock, PasswordManager, MFAHandler, AuditLogger,
//           CredentialManager, PolicyEngine, AnomalyDetector, GlobalNotifier,
//           PerformanceReport, UI helpers, and input validation
// ============================================================================
#include"Utilities.h"
#include"Users.h"
#include"MessageHandler.h"
#include<sstream>
#include<iomanip>
#include<cstdlib>
#include<cstring>
using namespace std;

// ======================== Clock ========================
Time Clock::now() {
    return time(nullptr);
}

// ======================== PasswordManager ========================
// Generate a random salt string matching the password length
string PasswordManager::gensalt(const string &password) {
    string salt;
    srand((unsigned)time(nullptr));
    for (size_t i = 0; i < password.size(); ++i) {
        salt += char(33 + rand() % 94);
    }
    return salt;
}

// XOR-based hash combining password and salt
string PasswordManager::hashpass(const string &password, const string &salt) {
    ostringstream out;
    for (size_t i = 0; i < password.size(); ++i) {
        unsigned char x = password[i] ^ salt[i % salt.size()];
        out << hex << setw(2) << setfill('0') << int(x);
    }
    return out.str();
}

// ======================== MFAHandler ========================
// Send a 6-digit OTP to a file (simulating secure delivery)
void MFAHandler::sendOTP(const string &user) {
    currentOTP = to_string(100000 + rand() % 900000);
    sentAt = Clock::now();
    ofstream out(otpFile);
    out << currentOTP;
    cout << Color::CYAN << "  [MFA] OTP saved to " << otpFile
         << " (expires in " << Ttl << "s)" << Color::RESET << "\n";
}

// Validate the OTP — must match and not be expired
bool MFAHandler::validateOTP(const string &in) {
    ifstream inF(otpFile);
    string fileOtp;
    if (!inF || !getline(inF, fileOtp)) return false;
    remove(otpFile);
    if (Clock::now() > sentAt + Ttl) return false;
    return in == fileOtp;
}

// ======================== AuditLogger ========================
// Standard log entry with timestamp
void AuditLogger::log(const string &username, const string &action,
                      const string &detail) {
    ofstream out(filename, ios::app);
    Time t = Clock::now();
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    out << "[" << buf << "] " << username << " " << action
        << " " << detail << "\n";
}

// Log a failed login attempt (clean format — no decorators)
void AuditLogger::logFailure(const string &username) {
    ofstream out(filename, ios::app);
    Time t = Clock::now();
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    out << "[" << buf << "] " << username << " LOGIN_FAILED\n";
}

// operator<< overload for structured logging (spec requirement)
AuditLogger& operator<<(AuditLogger &logger, const AuditEntry &entry) {
    ofstream out(logger.filename, ios::app);
    Time t = Clock::now();
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    out << "[" << buf << "] " << entry.username << " "
        << entry.action << " " << entry.detail << "\n";
    return logger;
}

const string& AuditLogger::getFilename() const { return filename; }

// ======================== CredentialManager ========================
CredentialManager::CredentialManager() : userCount(0), attemptCount(0) {}
CredentialManager::~CredentialManager() { clearAll(); }

void CredentialManager::clearAll() {
    for (int i = 0; i < userCount; ++i) delete users[i];
    userCount = 0;
}

// Load users from file (format: username:hash:salt:role)
void CredentialManager::load(const char* filename) {
    clearAll();
    ifstream in(filename);
    if (!in) return;
    char line[512];
    while (in.getline(line, 512) && userCount < MAX_USERS) {
        char uname[128], hash[128], salt[128], role[64];
        if (sscanf(line, "%127[^:]:%127[^:]:%127[^:]:%63s",
                   uname, hash, salt, role) != 4) continue;

        User* u = nullptr;
        if      (strcmp(role, "Junior")    == 0) u = new Junior(uname, hash, salt);
        else if (strcmp(role, "Employee")  == 0) u = new Employee(uname, hash, salt);
        else if (strcmp(role, "Manager")   == 0) u = new Manager(uname, hash, salt);
        else if (strcmp(role, "Director")  == 0) u = new Director(uname, hash, salt);
        else if (strcmp(role, "Executive") == 0) u = new Executive(uname, hash, salt);
        if (u) users[userCount++] = u;
    }
}

// Authenticate a user by username and password
User* CredentialManager::authenticate(const string &uname,
                                       const string &pwd) const {
    for (int i = 0; i < userCount; ++i) {
        if (users[i]->getUsername() == uname && users[i]->authenticate(pwd))
            return users[i];
    }
    return nullptr;
}

User** CredentialManager::getUsers(int &outCount) const {
    outCount = userCount;
    return const_cast<User**>(users);
}

// Add a new user and persist to file
bool CredentialManager::addUser(const string &uname, const string &hash,
                                const string &salt, const string &role,
                                const char* filename) {
    if (userCount >= MAX_USERS) return false;
    ofstream out(filename, ios::app);
    if (!out) return false;
    out << uname << ":" << hash << ":" << salt << ":" << role << "\n";

    User* u = nullptr;
    if      (role == "Junior")    u = new Junior(uname, hash, salt);
    else if (role == "Employee")  u = new Employee(uname, hash, salt);
    else if (role == "Manager")   u = new Manager(uname, hash, salt);
    else if (role == "Director")  u = new Director(uname, hash, salt);
    else if (role == "Executive") u = new Executive(uname, hash, salt);
    if (!u) return false;
    users[userCount++] = u;
    return true;
}

// --- Login lockout implementation ---
// Check if a username is locked out after MAX_ATTEMPTS failures
bool CredentialManager::isLocked(const string &username) const {
    for (int i = 0; i < attemptCount; ++i) {
        if (attempts[i].username == username &&
            attempts[i].failCount >= MAX_ATTEMPTS)
            return true;
    }
    return false;
}

// Record a failed login attempt
void CredentialManager::recordFailure(const string &username) {
    for (int i = 0; i < attemptCount; ++i) {
        if (attempts[i].username == username) {
            attempts[i].failCount++;
            return;
        }
    }
    if (attemptCount < MAX_USERS) {
        attempts[attemptCount].username = username;
        attempts[attemptCount].failCount = 1;
        attemptCount++;
    }
}

// Reset login attempts on successful login
void CredentialManager::resetAttempts(const string &username) {
    for (int i = 0; i < attemptCount; ++i) {
        if (attempts[i].username == username) {
            attempts[i].failCount = 0;
            return;
        }
    }
}

// ======================== PolicyEngine ========================
// All access control is centralized here — no hardcoded checks elsewhere

bool PolicyEngine::canCreate(const User* creator, const User* target) {
    return creator->getClearance() > target->getClearance();
}

bool PolicyEngine::canDelegate(const User* sender, const User* newAssignee) {
    return newAssignee->getClearance() >= sender->getClearance();
}

bool PolicyEngine::canSend(const User* sender, const User* recipient,
                           int typeCode) {
    // ALERT messages require Director+ clearance
    if (typeCode == 2) return sender->getClearance() >= CLEARANCE_DIRECTOR;
    return true;
}

bool PolicyEngine::canAddUser(const User* adder, int targetRole) {
    // Must be Manager+ to add users, and cannot create same/higher clearance
    return adder->getClearance() >= CLEARANCE_MANAGER &&
           targetRole < adder->getClearance();
}

bool PolicyEngine::canViewAllReports(const User* viewer) {
    return viewer->getClearance() >= CLEARANCE_DIRECTOR;
}

bool PolicyEngine::canViewAnomalies(const User* viewer) {
    return viewer->getClearance() >= CLEARANCE_MANAGER;
}

bool PolicyEngine::canClearAnomalies(const User* viewer) {
    return viewer->getClearance() >= CLEARANCE_MANAGER;
}

bool PolicyEngine::canSendNotification(const User* sender, int level) {
    // WARNING and EMERGENCY require Manager+
    if (level >= 1) return sender->getClearance() >= CLEARANCE_MANAGER;
    return true;
}

// operator() — functor for general permission checking (spec requirement)
bool PolicyEngine::operator()(const User* user, const string &action) const {
    if (action == "ADD_USER")         return user->getClearance() >= CLEARANCE_MANAGER;
    if (action == "VIEW_ANOMALIES")   return user->getClearance() >= CLEARANCE_MANAGER;
    if (action == "CLEAR_ANOMALIES")  return user->getClearance() >= CLEARANCE_MANAGER;
    if (action == "VIEW_ALL_REPORTS") return user->getClearance() >= CLEARANCE_DIRECTOR;
    if (action == "SEND_ALERT")       return user->getClearance() >= CLEARANCE_DIRECTOR;
    if (action == "SEND_WARNING")     return user->getClearance() >= CLEARANCE_MANAGER;
    if (action == "SEND_EMERGENCY")   return user->getClearance() >= CLEARANCE_MANAGER;
    return false;
}

// ======================== AnomalyDetector ========================
AnomalyDetector::AnomalyDetector(AuditLogger &l, MessageManager &m)
    : logger(l), msgs(m) {}

void AnomalyDetector::clearAnomalies() {
    remove("anomalyreport.txt");
    skipNextAlert = true;
    reported = false;
    lastClearTime = Clock::now();
}

// Scan audit.txt for suspicious patterns and generate anomaly report
void AnomalyDetector::scanAndReport(User* invoker, User** users,
                                     int userCount) {
    if (skipNextAlert) { skipNextAlert = false; return; }
    if (reported) return;

    ifstream in("audit.txt");
    if (!in) return;

    string line;
    int failCount = 0, reCount = 0, dfCount = 0;
    string failedUser[100], failedTime[100];
    string reUser[100], reTime[100];
    string dfUser[100], dfTime[100];

    while (getline(in, line)) {
        char ts[32], uname[64], action[32], detail[128];
        if (sscanf(line.c_str(), "[%31[^]]]%*c %63s %31s %127[^\n]",
                   ts, uname, action, detail) < 3) continue;

        // Parse timestamp to filter by lastClearTime
        tm t = {};
        int yy, mo, dd, hh, mi, ss;
        if (sscanf(ts, "%d-%d-%d %d:%d:%d",
                   &yy, &mo, &dd, &hh, &mi, &ss) == 6) {
            t.tm_year = yy - 1900; t.tm_mon = mo - 1; t.tm_mday = dd;
            t.tm_hour = hh; t.tm_min = mi; t.tm_sec = ss;
            Time entry = mktime(&t);
            if (entry <= lastClearTime) continue;
        }

        if (strcmp(action, "LOGIN_FAILED") == 0 &&
            failCount < MAX_LOGIN_ERRORS + 1) {
            failedUser[failCount] = uname;
            failedTime[failCount] = ts;
            ++failCount;
        } else if (strcmp(action, "DELEGATE_TASK") == 0 &&
                   reCount < MAX_REASSIGN + 1) {
            reUser[reCount] = uname;
            reTime[reCount] = ts;
            ++reCount;
        } else if (strcmp(action, "DELEGATE_FAILED") == 0) {
            dfUser[dfCount] = uname;
            dfTime[dfCount] = ts;
            ++dfCount;
        }
    }
    in.close();

    bool anomaly = (failCount > MAX_LOGIN_ERRORS) ||
                   (reCount > MAX_REASSIGN) || (dfCount > 0);
    if (!anomaly) return;

    // Generate anomaly report file
    Time now = Clock::now();
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));

    ofstream rpt("anomalyreport.txt");
    rpt << "=== Anomaly Report at [" << buf << "] ===\n";
    if (failCount > MAX_LOGIN_ERRORS) {
        rpt << "-- Failed logins (" << failCount
            << " > " << MAX_LOGIN_ERRORS << "):\n";
        for (int i = 0; i < failCount; ++i)
            rpt << "   [" << failedTime[i] << "] " << failedUser[i] << "\n";
    }
    if (reCount > MAX_REASSIGN) {
        rpt << "-- Reassignments (" << reCount
            << " > " << MAX_REASSIGN << "):\n";
        for (int i = 0; i < reCount; ++i)
            rpt << "   [" << reTime[i] << "] " << reUser[i] << "\n";
    }
    if (dfCount > 0) {
        rpt << "-- Delegation failures:\n";
        for (int i = 0; i < dfCount; ++i)
            rpt << "   [" << dfTime[i] << "] " << dfUser[i] << "\n";
    }
    rpt.close();

    // Send ALERT messages to all users
    User* sender = invoker ? invoker : users[0];
    for (int i = 0; i < userCount; ++i) {
        Message* m = new AlertMessage(sender, users[i],
                                       "[ANOMALY] See anomalyreport.txt");
        msgs.send(m);
    }
    msgs.save();

    printWarning("Suspicious activity detected! Check anomalyreport.txt");
    if (invoker) {
        printInfo("New Anomaly ALERT in your inbox");
    }
    reported = true;
}

// operator() — functor for anomaly scanning (spec requirement)
bool AnomalyDetector::operator()(User* invoker, User** users, int userCount) {
    bool prevReported = reported;
    scanAndReport(invoker, users, userCount);
    return reported && !prevReported; // true if new anomaly was found
}

// ======================== GlobalNotifier ========================
GlobalNotifier::GlobalNotifier(AuditLogger &l, MessageManager &m)
    : logger(l), msgs(m) {}

void GlobalNotifier::send(User* sender, GlobalLevel lvl, const string &text,
                          User** users, int userCount) {
    // Use PolicyEngine for permission check — no hardcoded clearance
    if (!PolicyEngine::canSendNotification(sender, (int)lvl)) {
        printError("Insufficient clearance to send this notification.");
        return;
    }
    logger.log(sender->getUsername(), "GLOBAL_NOTIFY", text);
    for (int i = 0; i < userCount; ++i) {
        Message* m = new InfoMessage(sender, users[i], text);
        msgs.send(m);
    }
    printSuccess("Global notification sent to all users.");
}

// ======================== PerformanceReport ========================
// operator<< as friend — generates performance output (spec requirement)
ostream& operator<<(ostream &os, const PerformanceReport &rpt) {
    int score = (rpt.onTime * 10) - (rpt.late * 10);
    os << Color::CYAN << "  Total Completed: " << Color::RESET << rpt.total << "\n"
       << Color::GREEN << "  On Time:         " << Color::RESET << rpt.onTime << "\n"
       << Color::RED   << "  Late:            " << Color::RESET << rpt.late << "\n"
       << Color::BYELLOW << "  Score:           " << Color::RESET << score << "\n";
    return os;
}

// operator+= — accumulate performance stats (spec requirement)
PerformanceReport& PerformanceReport::operator+=(const PerformanceReport &other) {
    total  += other.total;
    onTime += other.onTime;
    late   += other.late;
    return *this;
}

// Read a report for a specific user from progressreport.txt
PerformanceReport readReport(const string &user) {
    PerformanceReport rpt;
    ifstream in("progressreport.txt");
    string line;
    while (getline(in, line)) {
        size_t p1 = line.find(':'), p2 = line.find(':', p1 + 1),
               p3 = line.find(':', p2 + 1);
        if (p1 == string::npos || p2 == string::npos ||
            p3 == string::npos) continue;
        string u = line.substr(0, p1);
        if (u != user) continue;
        rpt.total  = stoi(line.substr(p1 + 1, p2 - p1 - 1));
        rpt.onTime = stoi(line.substr(p2 + 1, p3 - p2 - 1));
        rpt.late   = stoi(line.substr(p3 + 1));
        break;
    }
    return rpt;
}

// Update progress report after task completion
void updateReport(const char empID[], bool onTime) {
    ifstream fin("progressreport.txt");
    ofstream fout("progressreport_tmp.txt");
    char line[256];
    bool found = false;
    while (fin.getline(line, 256)) {
        char idbuf[50];
        int tot = 0, ot = 0, lt = 0;
        sscanf(line, "%49[^:]:%d:%d:%d", idbuf, &tot, &ot, &lt);
        if (strcmp(idbuf, empID) == 0) {
            found = true;
            ++tot;
            if (onTime) ++ot; else ++lt;
            fout << idbuf << ":" << tot << ":" << ot << ":" << lt << "\n";
        } else {
            fout << line << "\n";
        }
    }
    if (!found) {
        fout << empID << ":1:" << (onTime ? 1 : 0)
             << ":" << (onTime ? 0 : 1) << "\n";
    }
    fin.close(); fout.close();
    remove("progressreport.txt");
    rename("progressreport_tmp.txt", "progressreport.txt");
}

// ======================== Login Handler ========================
void handleLogin(User*& current, CredentialManager &creds, MFAHandler &mfa,
                 AuditLogger &logg, AnomalyDetector &anom) {
    string username = inputString("  Enter username: ");
    string password = inputString("  Enter password: ");

    // Check lockout BEFORE attempting authentication
    if (creds.isLocked(username)) {
        printError("Account locked! Max login attempts exceeded.");
        logg.logFailure(username);
        return;
    }

    User* tmp = creds.authenticate(username, password);
    if (!tmp) {
        creds.recordFailure(username);
        logg.logFailure(username);
        int uc; User** all = creds.getUsers(uc);
        anom.scanAndReport(nullptr, all, uc);
        printError("Invalid credentials.");
        return;
    }

    // MFA step
    mfa.sendOTP(username);
    string otp = inputString("  Enter OTP: ");
    if (!mfa.validateOTP(otp)) {
        creds.recordFailure(username);
        logg.logFailure(username);
        int uc; User** all = creds.getUsers(uc);
        anom.scanAndReport(nullptr, all, uc);
        printError("Invalid or expired OTP.");
        return;
    }

    // Successful login
    creds.resetAttempts(username);
    current = tmp;
    // Use operator<< for audit logging
    logg << AuditEntry{current->getUsername(), "LOGIN", "MFA OK"};

    int uc; User** all = creds.getUsers(uc);
    anom.scanAndReport(current, all, uc);

    cout << "\n" << Color::BGREEN << "  ✓ Welcome, "
         << current->getUsername() << "! ["
         << current->getRoleTitle() << "]" << Color::RESET << "\n\n";
}

// ======================== UI Helper Functions ========================
// Print a styled box title with ANSI colors
void printBoxTitle(const string &title) {
    const char *TL = "╔", *TR = "╗", *BL = "╚", *BR = "╝";
    const char *H = "═", *V = "║";
    int w = title.size() + 4;
    cout << Color::BCYAN << TL;
    for (int i = 0; i < w; ++i) cout << H;
    cout << TR << "\n" << V << "  " << Color::BOLD << Color::WHITE
         << title << Color::RESET << Color::BCYAN << "  " << V << "\n" << BL;
    for (int i = 0; i < w; ++i) cout << H;
    cout << BR << Color::RESET << "\n";
}

// Print colored text with prefix
void printColorLine(const string &text, const char* color) {
    cout << color << text << Color::RESET << "\n";
}

void printSuccess(const string &text) {
    cout << Color::BGREEN << "  ✓ " << text << Color::RESET << "\n";
}

void printError(const string &text) {
    cout << Color::BRED << "  ✗ " << text << Color::RESET << "\n";
}

void printWarning(const string &text) {
    cout << Color::BYELLOW << "  ⚠ " << text << Color::RESET << "\n";
}

void printInfo(const string &text) {
    cout << Color::BCYAN << "  ℹ " << text << Color::RESET << "\n";
}

// Validated integer input — prevents crashes on invalid input
int inputInt(const string &prompt) {
    cout << Color::WHITE << prompt << Color::RESET;
    int val;
    while (!(cin >> val)) {
        cin.clear();
        cin.ignore(1000, '\n');
        cout << Color::RED << "  Invalid input. " << Color::RESET << prompt;
    }
    return val;
}

// String input with prompt
string inputString(const string &prompt) {
    cout << Color::WHITE << prompt << Color::RESET;
    string val;
    cin >> val;
    return val;
}
