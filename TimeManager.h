// ============================================================================
// TimeManager.h — Separate TTL management class
// Handles task expiration with recursive traversal
// ============================================================================
#pragma once
#include<ctime>

class Task; // forward declaration

class TimeManager {
public:
    // Check if a single task's TTL has expired
    static bool isExpired(time_t createdAt, int ttl);
    // Recursively expire all overdue tasks in the linked list
    static void expireAll(Task* head);
private:
    // Recursive helper
    static void expireRec(Task* node);
};
