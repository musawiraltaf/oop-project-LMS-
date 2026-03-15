// ============================================================================
// TimeManager.cpp — Recursive TTL expiration logic
// Separated from TaskManager for better modularity
// ============================================================================
#include"TimeManager.h"
#include"TaskHandler.h"
#include"Utilities.h"

// Check if a single timestamp + TTL has expired
bool TimeManager::isExpired(time_t createdAt, int ttl) {
    return Clock::now() > createdAt + ttl;
}

// Public entry point — expire all tasks recursively
void TimeManager::expireAll(Task* head) {
    expireRec(head);
}

// Recursive traversal of task linked list
void TimeManager::expireRec(Task* node) {
    if (!node) return;  // base case

    // Only expire tasks that are not already completed or expired
    if (node->status != COMPLETED && node->status != EXPIRED) {
        if (isExpired(node->createdAt, node->ttl)) {
            node->status = EXPIRED;
        }
    }
    expireRec(node->next);  // recurse to next task
}
