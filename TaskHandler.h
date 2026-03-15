// ============================================================================
// TaskHandler.h — Task system with delegation, TTL, priorities, signatures
// Demonstrates: operator+=, operator<, operator==, friend functions,
//               digital signatures, cycle detection in delegation chains
// ============================================================================
#pragma once
#include<string>
#include"Users.h"
#include"Utilities.h"
using namespace std;

// ======================== Task Enums ========================
enum TaskStatus  { CREATED, ASSIGNED, INPROGRESS, COMPLETED, EXPIRED };
enum TaskPriority{ HIGH, MEDIUM, LOW };

// ======================== Task ========================
class Task {
public:
    int          id;
    User*        creator;
    User*        assignee;
    TaskStatus   status;
    TaskPriority priority;
    Time         createdAt;
    int          ttl;
    Task*        next;
    string       signature;  // digital signature: hash(creator + timestamp)

    // Delegation chain for cycle detection (bonus feature)
    static const int MAX_CHAIN = 20;
    string delegationChain[MAX_CHAIN];
    int    chainLength;

    Task(int i, User* c, User* a, int ttl, TaskPriority p);

    // Check if a user is already in the delegation chain
    bool isInChain(const string &username) const;
    // Add a user to the delegation chain
    void addToChain(const string &username);

    // Operator overloads
    friend bool operator<(const Task &a, const Task &b);
    friend bool operator==(const Task &a, const Task &b);
};

// ======================== TaskManager ========================
class TaskManager {
public:
    enum DelegateResult { SUCCESS, NOT_OWNER, NO_CLEARANCE, CYCLE_DETECTED };
private:
    Task* head;
    const char* taskFile;
    int nextId;

    // Recursive delegation traversal
    DelegateResult delegateRec(Task* node, int id, User* cur, User* nxt);
public:
    TaskManager(const char* filename = "tasks.txt");
    ~TaskManager();

    // Core operations
    Task* createTask(User* creator, User* assignee, int ttl, TaskPriority p);
    Task* find(int id);
    void  expireAll();
    DelegateResult delegateTask(int taskId, User* cur, User* nxt);
    bool  startTask(int taskId, User* u);   // transition ASSIGNED → INPROGRESS
    void  listSortedFor(User* u);
    void  save();
    void  load(User** users, int userCount);
    bool  hasHigherIncomplete(int prio, User* u);
    void  cleanup();

    // operator+= — add a task to the manager (spec requirement)
    TaskManager& operator+=(Task* t);

    // Getter for head (used by TimeManager)
    Task* getHead() const;
};
