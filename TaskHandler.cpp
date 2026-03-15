// ============================================================================
// TaskHandler.cpp — Task system implementation
// Features: TTL expiration, delegation with cycle detection,
//           digital signatures, priority sorting, operator+=
// ============================================================================
#include"TaskHandler.h"
#include"TimeManager.h"
#include"Utilities.h"
#include<fstream>
#include<iostream>
using namespace std;

// ======================== Task ========================
Task::Task(int i, User* c, User* a, int t, TaskPriority p)
    : id(i), creator(c), assignee(a), status(ASSIGNED), priority(p),
      createdAt(Clock::now()), ttl(t), next(nullptr), chainLength(0) {
    // Generate digital signature: hash(creator_username + timestamp)
    string sigData = c->getUsername() + to_string(createdAt);
    signature = PasswordManager::hashpass(sigData, "TASK_SIG");

    // Initialize delegation chain with creator and first assignee
    delegationChain[chainLength++] = c->getUsername();
    if (a->getUsername() != c->getUsername()) {
        delegationChain[chainLength++] = a->getUsername();
    }
}

// Check if a username exists in the delegation chain (cycle detection)
bool Task::isInChain(const string &username) const {
    for (int i = 0; i < chainLength; ++i) {
        if (delegationChain[i] == username) return true;
    }
    return false;
}

// Add a username to the delegation chain
void Task::addToChain(const string &username) {
    if (chainLength < MAX_CHAIN) {
        delegationChain[chainLength++] = username;
    }
}

// operator< — sort by priority first, then by creation time
bool operator<(const Task &a, const Task &b) {
    return a.priority != b.priority ? a.priority < b.priority
                                    : a.createdAt < b.createdAt;
}

// operator== — tasks are equal if they share the same ID
bool operator==(const Task &a, const Task &b) {
    return a.id == b.id;
}

// ======================== TaskManager ========================
TaskManager::TaskManager(const char* fn)
    : head(nullptr), taskFile(fn), nextId(1) {}

TaskManager::~TaskManager() { cleanup(); }

// Recursive delegation — traverses linked list to find and delegate task
TaskManager::DelegateResult
TaskManager::delegateRec(Task* node, int id, User* cur, User* nxt) {
    if (!node) return NOT_OWNER;
    if (node->id == id) {
        if (node->assignee != cur) return NOT_OWNER;
        // Full cycle detection — check entire delegation chain
        if (node->isInChain(nxt->getUsername())) return CYCLE_DETECTED;
        if (!PolicyEngine::canDelegate(cur, nxt)) return NO_CLEARANCE;
        node->assignee = nxt;
        node->addToChain(nxt->getUsername());
        return SUCCESS;
    }
    return delegateRec(node->next, id, cur, nxt);
}

// Create a new task with digital signature
Task* TaskManager::createTask(User* creator, User* assignee,
                               int ttl, TaskPriority p) {
    Task* t = new Task(nextId++, creator, assignee, ttl, p);
    *this += t;  // use operator+= to add task
    return t;
}

// Find a task by ID
Task* TaskManager::find(int id) {
    for (Task* p = head; p; p = p->next)
        if (p->id == id) return p;
    return nullptr;
}

// Expire all overdue tasks using TimeManager (separate class)
void TaskManager::expireAll() {
    TimeManager::expireAll(head);
}

// Delegate a task to a new assignee
TaskManager::DelegateResult
TaskManager::delegateTask(int tid, User* cur, User* nxt) {
    return delegateRec(head, tid, cur, nxt);
}

// Transition task from ASSIGNED → INPROGRESS
bool TaskManager::startTask(int taskId, User* u) {
    Task* t = find(taskId);
    if (!t) return false;
    if (t->assignee != u) return false;
    if (t->status != ASSIGNED) return false;
    t->status = INPROGRESS;
    return true;
}

// List tasks for a user, sorted by priority using selection sort
void TaskManager::listSortedFor(User* u) {
    // Count relevant tasks
    int cnt = 0;
    for (Task* p = head; p; p = p->next) {
        if (p->creator == u || p->assignee == u) ++cnt;
    }
    if (!cnt) {
        printInfo("No tasks found.");
        return;
    }

    // Collect into dynamic array
    Task** arr = new Task*[cnt];
    int idx = 0;
    for (Task* p = head; p; p = p->next) {
        if (p->creator == u || p->assignee == u)
            arr[idx++] = p;
    }

    // Selection sort by priority
    for (int i = 0; i < cnt; ++i) {
        int best = i;
        for (int j = i + 1; j < cnt; ++j) {
            if (*arr[j] < *arr[best]) best = j;
        }
        Task* tmp = arr[i]; arr[i] = arr[best]; arr[best] = tmp;
    }

    // Status names for display
    const char* statusNames[] = {"CREATED","ASSIGNED","IN-PROGRESS",
                                  "COMPLETED","EXPIRED"};
    const char* prioNames[]   = {"HIGH","MEDIUM","LOW"};
    const char* prioColors[]  = {Color::BRED, Color::BYELLOW, Color::BGREEN};
    const char* statusColors[] = {Color::WHITE, Color::CYAN, Color::BLUE,
                                   Color::GREEN, Color::RED};

    // Display tasks in a formatted table
    cout << Color::BCYAN
         << "  ┌────┬──────────┬──────────┬────────────┬────────┬──────────────┐\n"
         << "  │ ID │ Creator  │ Assignee │ Status     │ Prio   │ Signature    │\n"
         << "  ├────┼──────────┼──────────┼────────────┼────────┼──────────────┤\n"
         << Color::RESET;

    for (int i = 0; i < cnt; ++i) {
        Task* t = arr[i];
        int si = (int)t->status;
        int pi = (int)t->priority;
        string sigShort = t->signature.substr(0, 10) + "..";

        cout << Color::CYAN << "  │ " << Color::WHITE << t->id;
        // Pad ID
        if (t->id < 10) cout << " ";
        cout << Color::CYAN << " │ " << Color::WHITE;
        // Pad creator username to 8
        string cname = t->creator->getUsername();
        cout << cname;
        for (int s = cname.size(); s < 8; ++s) cout << " ";
        cout << Color::CYAN << " │ " << Color::WHITE;
        // Pad assignee username to 8
        string aname = t->assignee->getUsername();
        cout << aname;
        for (int s = aname.size(); s < 8; ++s) cout << " ";
        cout << Color::CYAN << " │ " << statusColors[si]
             << statusNames[si];
        int slen = 0;
        for (const char* c = statusNames[si]; *c; ++c) slen++;
        for (int s = slen; s < 10; ++s) cout << " ";
        cout << Color::CYAN << " │ " << prioColors[pi]
             << prioNames[pi];
        int plen = 0;
        for (const char* c = prioNames[pi]; *c; ++c) plen++;
        for (int s = plen; s < 6; ++s) cout << " ";
        cout << Color::CYAN << " │ " << Color::DIM << sigShort;
        for (int s = sigShort.size(); s < 12; ++s) cout << " ";
        cout << Color::CYAN << " │" << Color::RESET << "\n";
    }

    cout << Color::BCYAN
         << "  └────┴──────────┴──────────┴────────────┴────────┴──────────────┘"
         << Color::RESET << "\n";

    delete[] arr;
}

// Save tasks to file
void TaskManager::save() {
    ofstream out(taskFile);
    for (Task* p = head; p; p = p->next) {
        if (p->status != COMPLETED)
            out << p->id << ","
                << p->creator->getUsername() << ","
                << p->assignee->getUsername() << ","
                << (int)p->status << ","
                << (int)p->priority << ","
                << p->createdAt << ","
                << p->ttl << ","
                << p->signature << "\n";
    }
}

// Load tasks from file, reconstruct Task objects
void TaskManager::load(User** users, int userCount) {
    cleanup();
    ifstream in(taskFile);
    string line;
    while (getline(in, line)) {
        // Parse: id,creator,assignee,status,priority,createdAt,ttl,signature
        size_t pos = 0, nxt;
        string parts[8];
        for (int i = 0; i < 7; ++i) {
            nxt = line.find(',', pos);
            if (nxt == string::npos) break;
            parts[i] = line.substr(pos, nxt - pos);
            pos = nxt + 1;
        }
        parts[7] = line.substr(pos);

        if (parts[0].empty()) continue;
        int id_          = stoi(parts[0]);
        string creatorU  = parts[1];
        string assigneeU = parts[2];
        TaskStatus st    = (TaskStatus)stoi(parts[3]);
        TaskPriority pr  = (TaskPriority)stoi(parts[4]);
        Time ct          = (Time)stoll(parts[5]);
        int tttl         = stoi(parts[6]);
        string sig       = parts[7];

        // Resolve user pointers
        User* cr = nullptr;
        User* as = nullptr;
        for (int i = 0; i < userCount; ++i) {
            if (users[i]->getUsername() == creatorU)  cr = users[i];
            if (users[i]->getUsername() == assigneeU) as = users[i];
        }
        if (!cr || !as) continue;

        Task* x = new Task(id_, cr, as, tttl, pr);
        x->status    = st;
        x->createdAt = ct;
        if (!sig.empty()) x->signature = sig;
        x->next = head;
        head = x;
        if (id_ >= nextId) nextId = id_ + 1;
    }
}

// Check if user has higher-priority incomplete tasks
bool TaskManager::hasHigherIncomplete(int prio, User* u) {
    for (Task* p = head; p; p = p->next) {
        if (p->assignee == u && (int)p->priority < prio &&
            p->status != COMPLETED && p->status != EXPIRED)
            return true;
    }
    return false;
}

// operator+= — add a task to the linked list (spec requirement)
TaskManager& TaskManager::operator+=(Task* t) {
    t->next = head;
    head = t;
    return *this;
}

// Get head pointer (for TimeManager)
Task* TaskManager::getHead() const { return head; }

// Free all task memory
void TaskManager::cleanup() {
    while (head) {
        Task* nxt = head->next;
        delete head;
        head = nxt;
    }
}
