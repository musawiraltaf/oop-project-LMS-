// ============================================================================
// Main.cpp — Entry point for OSIM system
// Organizational Simulation and Internal Management System
// Features: Colored menus, validated input, PolicyEngine checks,
//           full task lifecycle, dashboard, all features demonstrated
// ============================================================================
#include"Utilities.h"
#include"Users.h"
#include"TaskHandler.h"
#include"MessageHandler.h"
#include<iostream>
#include<fstream>
#include<iomanip>
#include<ctime>
#include<cstring>
#include<cstdlib>
#include<string>
using namespace std;

// ======================== Menu Display ========================
void showMainMenu(User* current) {
    const int W = 60;
    const char *TL = "╔", *TR = "╗", *BL = "╚", *BR = "╝";
    const char *H  = "═", *V  = "║", *ML = "╠", *MR = "╣";

    // Top border
    cout << "\n" << Color::BCYAN << "  " << TL;
    for (int i = 0; i < W; ++i) cout << H;
    cout << TR << Color::RESET << "\n";

    // Title
    string title = "OSIM  -  MAIN MENU";
    int pad = (W - title.size()) / 2;
    cout << Color::BCYAN << "  " << V << Color::RESET;
    for (int i = 0; i < pad; ++i) cout << " ";
    cout << Color::BOLD << Color::WHITE << title << Color::RESET;
    for (int i = 0; i < W - pad - (int)title.size(); ++i) cout << " ";
    cout << Color::BCYAN << V << Color::RESET << "\n";

    // User info line
    string userInfo = current->getUsername() + " [" +
                      current->getRoleTitle() + "] | Clearance: " +
                      to_string(current->getClearance());
    pad = (W - userInfo.size()) / 2;
    cout << Color::BCYAN << "  " << V << Color::RESET;
    for (int i = 0; i < pad; ++i) cout << " ";
    cout << Color::CYAN << userInfo << Color::RESET;
    for (int i = 0; i < W - pad - (int)userInfo.size(); ++i) cout << " ";
    cout << Color::BCYAN << V << Color::RESET << "\n";

    // Separator
    cout << Color::BCYAN << "  " << ML;
    for (int i = 0; i < W; ++i) cout << H;
    cout << MR << Color::RESET << "\n";

    // Menu items (two columns)
    const char* items[] = {
        " [1]  Switch User",        " [2]  Create Task",
        " [3]  Delegate Task",      " [4]  Start Task",
        " [5]  Complete Task",      " [6]  List Users",
        " [7]  Add User",           " [8]  My Tasks",
        " [9]  Send Message",       "[10]  Inbox",
        "[11]  Progress Report",    "[12]  Anomaly Report",
        "[13]  Clear Anomalies",    "[14]  Global Notify",
        "[15]  Dashboard",          "[16]  Exit"
    };
    const char* itemColors[] = {
        Color::YELLOW, Color::GREEN,
        Color::GREEN,  Color::GREEN,
        Color::GREEN,  Color::CYAN,
        Color::CYAN,   Color::CYAN,
        Color::MAGENTA, Color::MAGENTA,
        Color::BLUE,   Color::BLUE,
        Color::BLUE,   Color::MAGENTA,
        Color::BYELLOW, Color::RED
    };

    for (int i = 0; i < 16; i += 2) {
        cout << Color::BCYAN << "  " << V << Color::RESET << "  ";
        // Left item
        cout << itemColors[i] << items[i] << Color::RESET;
        int leftLen = 0;
        for (const char* c = items[i]; *c; ++c) leftLen++;
        for (int s = leftLen; s < 26; ++s) cout << " ";
        // Right item
        cout << itemColors[i + 1] << items[i + 1] << Color::RESET;
        int rightLen = 0;
        for (const char* c = items[i + 1]; *c; ++c) rightLen++;
        for (int s = rightLen; s < W - 28; ++s) cout << " ";
        cout << Color::BCYAN << V << Color::RESET << "\n";
    }

    // Bottom border
    cout << Color::BCYAN << "  " << BL;
    for (int i = 0; i < W; ++i) cout << H;
    cout << BR << Color::RESET << "\n";
}

// ======================== Dashboard Display ========================
void showDashboard(User* current, TaskManager &tasks, MessageManager &msgs) {
    printBoxTitle("DASHBOARD - " + current->getUsername());

    // Show role info using virtual polymorphism
    current->displayDashboard();
    cout << "\n";

    // Count tasks by status
    int totalTasks = 0, assigned = 0, inprog = 0, completed = 0, expired = 0;
    Task* t = tasks.getHead();
    while (t) {
        if (t->creator == current || t->assignee == current) {
            totalTasks++;
            if (t->status == ASSIGNED)   assigned++;
            if (t->status == INPROGRESS) inprog++;
            if (t->status == COMPLETED)  completed++;
            if (t->status == EXPIRED)    expired++;
        }
        t = t->next;
    }

    cout << Color::BCYAN << "  ┌─── Task Summary ──────────────────┐\n"
         << Color::RESET;
    cout << "  " << Color::CYAN << "│" << Color::RESET
         << " Total: " << Color::WHITE << totalTasks
         << Color::RESET << "  "
         << Color::CYAN << "Assigned: " << Color::WHITE << assigned
         << Color::RESET << "  "
         << Color::BLUE << "Active: " << Color::WHITE << inprog
         << Color::RESET << "\n";
    cout << "  " << Color::CYAN << "│" << Color::RESET
         << " " << Color::GREEN << "Done: " << Color::WHITE << completed
         << Color::RESET << "   "
         << Color::RED << "Expired: " << Color::WHITE << expired
         << Color::RESET << "\n";
    cout << Color::BCYAN << "  └────────────────────────────────────┘\n"
         << Color::RESET;

    // Performance report
    cout << "\n";
    auto rpt = readReport(current->getUsername());
    cout << Color::BCYAN << "  ┌─── Performance ──────────────────┐\n"
         << Color::RESET;
    cout << rpt;
    cout << Color::BCYAN << "  └────────────────────────────────────┘\n"
         << Color::RESET;
}

// ======================== Main Entry Point ========================
int main() {
    srand((unsigned)time(nullptr));

    // Initialize core systems
    CredentialManager creds;
    creds.load("users.txt");
    int userCount;
    User** users = creds.getUsers(userCount);
    if (userCount == 0) {
        printError("No users loaded. Exiting.");
        return 1;
    }

    MFAHandler mfa;
    AuditLogger logger;
    MessageManager msgs("inbox.txt");
    msgs.load(users, userCount);
    TaskManager tasks("tasks.txt");
    tasks.load(users, userCount);
    AnomalyDetector anom(logger, msgs);
    GlobalNotifier notifier(logger, msgs);
    PolicyEngine policy;  // instance for operator() usage

    // ── Login ──
    User* current = nullptr;
    while (!current) {
        printBoxTitle("OSIM - LOGIN REQUIRED");
        handleLogin(current, creds, mfa, logger, anom);
    }
    logger.log(current->getUsername(), "START", "Session begins");

    // ── Main Loop ──
    bool running = true;
    while (running) {
        tasks.expireAll();
        anom.scanAndReport(current, users, userCount);

        showMainMenu(current);
        int choice = inputInt("  Select option: ");

        switch (choice) {

        // ────────────────── 1: Switch User ──────────────────
        case 1: {
            current = nullptr;
            while (!current) {
                printBoxTitle("SWITCH USER");
                handleLogin(current, creds, mfa, logger, anom);
            }
            logger << AuditEntry{current->getUsername(), "SWITCH",
                                 "User switched"};
            break;
        }

        // ────────────────── 2: Create Task ──────────────────
        case 2: {
            printBoxTitle("CREATE TASK");
            string assigneeName = inputString("  Assignee username: ");
            int ttlSec          = inputInt("  Duration (seconds): ");
            int prioVal         = inputInt("  Priority (0=HIGH, 1=MEDIUM, 2=LOW): ");

            if (prioVal < 0 || prioVal > 2) {
                printError("Invalid priority. Must be 0, 1, or 2.");
                break;
            }

            // Find assignee user
            User* assigneePtr = nullptr;
            for (int i = 0; i < userCount; ++i) {
                if (users[i]->getUsername() == assigneeName)
                    assigneePtr = users[i];
            }
            if (!assigneePtr) { printError("User not found."); break; }

            // Centralized permission check via PolicyEngine
            if (!PolicyEngine::canCreate(current, assigneePtr)) {
                printError("Insufficient clearance to assign to this user.");
                break;
            }

            Task* newTask = tasks.createTask(current, assigneePtr, ttlSec,
                                              (TaskPriority)prioVal);
            printSuccess("Task #" + to_string(newTask->id) + " created. Sig: "
                         + newTask->signature.substr(0, 12) + "..");

            // Notify assignee via INFO message
            msgs.send(new InfoMessage(current, assigneePtr,
                      "Task " + to_string(newTask->id) + " assigned to you"));
            logger.log(current->getUsername(), "CREATE_TASK",
                       to_string(newTask->id));
            break;
        }

        // ────────────────── 3: Delegate Task ──────────────────
        case 3: {
            printBoxTitle("DELEGATE TASK");
            int taskId = inputInt("  Task ID: ");
            Task* taskPtr = tasks.find(taskId);
            if (!taskPtr) { printError("Task not found."); break; }
            if (taskPtr->assignee != current) {
                printError("You are not the current assignee."); break;
            }

            string newAssigneeName = inputString("  New assignee: ");
            User* newAssigneePtr = nullptr;
            for (int i = 0; i < userCount; ++i) {
                if (users[i]->getUsername() == newAssigneeName) {
                    newAssigneePtr = users[i];
                    break;
                }
            }
            if (!newAssigneePtr) { printError("User not found."); break; }

            // Attempt delegation (includes cycle detection)
            TaskManager::DelegateResult res =
                tasks.delegateTask(taskId, current, newAssigneePtr);

            if (res == TaskManager::CYCLE_DETECTED) {
                // Log anomaly for cyclic delegation
                logger.logFailure(current->getUsername());
                Time now = Clock::now();
                char ts[64];
                strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
                ofstream aout("anomalyreport.txt", ios::app);
                aout << "[" << ts << "] " << current->getUsername()
                     << " DELEGATE_CYCLE " << newAssigneePtr->getUsername()
                     << "\n";
                msgs.send(new AlertMessage(current, current,
                    "[ANOMALY] Cyclic delegation detected to "
                    + newAssigneePtr->getUsername()));
                msgs.save();
                printError("Cyclic delegation blocked! See anomalyreport.txt");
                break;
            }

            if (res == TaskManager::NO_CLEARANCE) {
                logger.logFailure(current->getUsername());
                Time now = Clock::now();
                char ts[64];
                strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
                ofstream aout("anomalyreport.txt", ios::app);
                aout << "[" << ts << "] " << current->getUsername()
                     << " DELEGATE_LOWCLEARANCE "
                     << newAssigneePtr->getUsername() << "\n";
                msgs.send(new AlertMessage(current, current,
                    "[ANOMALY] Delegation to lower-clearance user blocked"));
                msgs.save();
                printError("Delegation blocked: insufficient clearance.");
                break;
            }

            if (res == TaskManager::NOT_OWNER) {
                printError("You are not the task owner."); break;
            }

            printSuccess("Task #" + to_string(taskId) + " delegated to "
                         + newAssigneeName);
            msgs.send(new InfoMessage(current, newAssigneePtr,
                "Task " + to_string(taskId) + " delegated to you"));
            logger.log(current->getUsername(), "DELEGATE_TASK",
                       to_string(taskId));
            break;
        }

        // ────────────────── 4: Start Task ──────────────────
        case 4: {
            printBoxTitle("START TASK");
            int taskId = inputInt("  Task ID: ");
            if (tasks.startTask(taskId, current)) {
                printSuccess("Task #" + to_string(taskId) + " is now IN-PROGRESS.");
                logger.log(current->getUsername(), "START_TASK",
                           to_string(taskId));
            } else {
                printError("Cannot start task. Check ID, ownership, and status.");
            }
            break;
        }

        // ────────────────── 5: Complete Task ──────────────────
        case 5: {
            printBoxTitle("COMPLETE TASK");
            int taskId = inputInt("  Task ID: ");
            Task* taskPtr = tasks.find(taskId);
            if (!taskPtr || taskPtr->assignee != current) {
                printError("Task not assigned to you!");
                break;
            }
            if (taskPtr->status == EXPIRED) {
                printError("Task has expired and cannot be completed.");
                break;
            }
            if (tasks.hasHigherIncomplete((int)taskPtr->priority, current)) {
                printWarning("Complete higher-priority tasks first.");
                break;
            }
            bool onTime = (Clock::now() <= taskPtr->createdAt + taskPtr->ttl);
            taskPtr->status = COMPLETED;
            printSuccess("Task #" + to_string(taskId) + " completed"
                         + (onTime ? " on time!" : " (late)"));
            updateReport(current->getUsername().c_str(), onTime);
            logger.log(current->getUsername(), "COMPLETE_TASK",
                       to_string(taskId));
            break;
        }

        // ────────────────── 6: List Users ──────────────────
        case 6: {
            printBoxTitle("USER LIST");
            if (userCount == 0) {
                printInfo("No users."); break;
            }
            // Copy pointers for sorting
            User* arr[100];
            for (int i = 0; i < userCount; ++i) arr[i] = users[i];
            // Selection sort by username
            for (int i = 0; i < userCount; ++i) {
                int best = i;
                for (int j = i + 1; j < userCount; ++j) {
                    if (arr[j]->getUsername() < arr[best]->getUsername())
                        best = j;
                }
                User* tmp = arr[i]; arr[i] = arr[best]; arr[best] = tmp;
            }
            // Display with role info (polymorphic getRoleTitle)
            cout << Color::BCYAN
                 << "  ┌──────────────┬────────────┬───────────┐\n"
                 << "  │ Username     │ Role       │ Clearance │\n"
                 << "  ├──────────────┼────────────┼───────────┤\n"
                 << Color::RESET;
            for (int i = 0; i < userCount; ++i) {
                string name = arr[i]->getUsername();
                string role = arr[i]->getRoleTitle(); // polymorphic call
                int clr     = arr[i]->getClearance();
                cout << Color::CYAN << "  │ " << Color::WHITE << name;
                for (int s = name.size(); s < 12; ++s) cout << " ";
                cout << Color::CYAN << " │ " << Color::YELLOW << role;
                for (int s = role.size(); s < 10; ++s) cout << " ";
                cout << Color::CYAN << " │ " << Color::WHITE << "    " << clr
                     << "      " << Color::CYAN << "│" << Color::RESET << "\n";
            }
            cout << Color::BCYAN
                 << "  └──────────────┴────────────┴───────────┘\n"
                 << Color::RESET;
            break;
        }

        // ────────────────── 7: Add User ──────────────────
        case 7: {
            printBoxTitle("ADD USER");
            // Use PolicyEngine operator() functor — no hardcoded clearance check
            if (!policy(current, "ADD_USER")) {
                printError("Insufficient clearance (Manager+ required).");
                break;
            }
            string newUsername = inputString("  New username: ");

            // Check for duplicates
            int existingCount;
            User** allUsers = creds.getUsers(existingCount);
            bool duplicate = false;
            for (int i = 0; i < existingCount; ++i) {
                if (allUsers[i]->getUsername() == newUsername) {
                    duplicate = true; break;
                }
            }
            if (duplicate) { printError("Username already exists."); break; }

            string newPassword = inputString("  Password: ");
            int roleChoice = inputInt("  Role (1=Junior, 2=Employee, 3=Manager): ");

            // Validate via PolicyEngine
            if (!PolicyEngine::canAddUser(current, roleChoice)) {
                printError("Cannot create user with same or higher clearance.");
                break;
            }

            string roleName = (roleChoice == 1 ? "Junior" :
                               roleChoice == 2 ? "Employee" : "Manager");
            string salt = PasswordManager::gensalt(newPassword);
            string hash = PasswordManager::hashpass(newPassword, salt);
            creds.addUser(newUsername, hash, salt, roleName, "users.txt");
            printSuccess("User '" + newUsername + "' added as " + roleName);
            users = creds.getUsers(userCount);
            break;
        }

        // ────────────────── 8: My Tasks ──────────────────
        case 8: {
            printBoxTitle("MY TASKS");
            tasks.listSortedFor(current);
            break;
        }

        // ────────────────── 9: Send Message ──────────────────
        case 9: {
            printBoxTitle("SEND MESSAGE");
            string recipientName = inputString("  Recipient: ");
            int msgTypeVal = inputInt("  Type (0=INFO, 1=PRIVATE, 2=ALERT): ");

            if (msgTypeVal < 0 || msgTypeVal > 2) {
                printError("Invalid message type.");
                break;
            }

            cin.ignore();
            cout << Color::WHITE << "  Message: " << Color::RESET;
            string msgText;
            getline(cin, msgText);

            User* recipientPtr = nullptr;
            for (int i = 0; i < userCount; ++i) {
                if (users[i]->getUsername() == recipientName) {
                    recipientPtr = users[i];
                }
            }
            if (!recipientPtr) { printError("Recipient not found."); break; }

            // Create appropriate message subclass (polymorphism)
            Message* newMsg = nullptr;
            if (msgTypeVal == 0) {
                newMsg = new InfoMessage(current, recipientPtr, msgText);
            } else if (msgTypeVal == 1) {
                newMsg = new PrivateMessage(current, recipientPtr, msgText);
            } else if (msgTypeVal == 2) {
                newMsg = new AlertMessage(current, recipientPtr, msgText);
            }

            // Centralized permission check via PolicyEngine
            if (!PolicyEngine::canSend(current, recipientPtr, msgTypeVal)) {
                printError("Insufficient clearance for this message type.");
                delete newMsg;
            } else {
                msgs.send(newMsg);
                printSuccess("Message sent to " + recipientName);
                logger.log(current->getUsername(), "SEND_MESSAGE",
                           recipientName);
            }
            break;
        }

        // ────────────────── 10: Inbox ──────────────────
        case 10: {
            printBoxTitle("INBOX");
            msgs.listFor(current);
            break;
        }

        // ────────────────── 11: Progress Report ──────────────────
        case 11: {
            printBoxTitle("PROGRESS REPORT");
            // Use PolicyEngine — no hardcoded clearance check
            if (PolicyEngine::canViewAllReports(current)) {
                cout << Color::BCYAN << "  ═══ All Progress Reports ═══"
                     << Color::RESET << "\n";
                ifstream in("progressreport.txt");
                string line;
                while (getline(in, line)) {
                    cout << "  " << line << "\n";
                }
            } else {
                auto rpt = readReport(current->getUsername());
                cout << rpt;
            }
            break;
        }

        // ────────────────── 12: Anomaly Report ──────────────────
        case 12: {
            printBoxTitle("ANOMALY REPORT");
            // Use PolicyEngine — no hardcoded clearance check
            if (!PolicyEngine::canViewAnomalies(current)) {
                printError("Access denied (Manager+ required).");
                break;
            }
            ifstream in("anomalyreport.txt");
            if (!in) {
                printInfo("No anomaly report found.");
                break;
            }
            cout << Color::BYELLOW << "  ═══ Anomaly Report ═══"
                 << Color::RESET << "\n";
            string line;
            while (getline(in, line)) {
                cout << Color::YELLOW << "  " << line
                     << Color::RESET << "\n";
            }
            break;
        }

        // ────────────────── 13: Clear Anomalies ──────────────────
        case 13: {
            // Use PolicyEngine — no hardcoded clearance check
            if (!PolicyEngine::canClearAnomalies(current)) {
                printError("Access denied (Manager+ required).");
                break;
            }
            anom.clearAnomalies();
            printSuccess("Anomalies cleared.");
            break;
        }

        // ────────────────── 14: Global Notify ──────────────────
        case 14: {
            printBoxTitle("GLOBAL NOTIFICATION");
            int levelVal = inputInt("  Level (0=INFO, 1=WARNING, 2=EMERGENCY): ");
            if (levelVal < 0 || levelVal > 2) {
                printError("Invalid notification level."); break;
            }
            cin.ignore();
            cout << Color::WHITE << "  Text: " << Color::RESET;
            string notifText;
            getline(cin, notifText);
            notifier.send(current, (GlobalLevel)levelVal, notifText,
                          users, userCount);
            break;
        }

        // ────────────────── 15: Dashboard ──────────────────
        case 15: {
            showDashboard(current, tasks, msgs);
            break;
        }

        // ────────────────── 16: Exit ──────────────────
        case 16: {
            running = false;
            logger.log(current->getUsername(), "EXIT", "Session ends");
            cout << "\n" << Color::BGREEN
                 << "  Goodbye, " << current->getUsername() << "!"
                 << Color::RESET << "\n\n";
            break;
        }

        default:
            printError("Invalid choice. Please select 1-16.");
        }

        // Persist state after every action
        tasks.save();
        msgs.save();
    }
    return 0;
}