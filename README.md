# 🏢 OSIM — Organizational Simulation & Internal Management System

> A feature-rich, console-based organizational management simulator built in **C++**, demonstrating advanced Object-Oriented Programming principles including multi-level inheritance, runtime polymorphism, operator overloading, and secure credential management.

---

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Class Hierarchy](#class-hierarchy)
- [OOP Concepts Demonstrated](#oop-concepts-demonstrated)
- [Getting Started](#getting-started)
- [Default Users](#default-users)
- [Usage Guide](#usage-guide)
- [Project Structure](#project-structure)
- [Data Files](#data-files)
- [Technologies Used](#technologies-used)
- [License](#license)

---

## 🔍 Overview

OSIM is a terminal-based organizational simulation system that models a hierarchical workplace environment. Users can log in with role-based access, create and delegate tasks, exchange secure messages, monitor anomalies, and view real-time dashboards — all governed by a centralized **PolicyEngine** that enforces clearance-based permissions.

The system features colorful ANSI-styled menus with Unicode box-drawing characters for a polished console UI experience.

---

## ✨ Features

### 🔐 Authentication & Security
- **Password hashing** with salt-based XOR encryption (`PasswordManager`)
- **Multi-Factor Authentication (MFA)** via OTP generation and validation
- **Login lockout** — account locks after 3 consecutive failed attempts
- **Audit logging** — every action is timestamped and recorded to an immutable log

### 👥 Role-Based Access Control
- **5-level inheritance chain**: Junior → Employee → Manager → Director → Executive
- Each role has distinct clearance levels (1–5) and explicit permissions
- Centralized permission enforcement via the **PolicyEngine** (no hardcoded checks)

### 📝 Task Management
- **Full task lifecycle**: Created → Assigned → In-Progress → Completed / Expired
- **Task delegation** with cycle detection (prevents circular assignment chains)
- **TTL-based auto-expiration** managed by a dedicated `TimeManager` class
- **Priority system** (High / Medium / Low) with priority-based completion ordering
- **Digital signatures** — each task is signed with a hash of creator + timestamp

### 💬 Messaging System
- **Three message types** with polymorphic behavior:
  - `InfoMessage` — general notifications
  - `PrivateMessage` — XOR-encrypted content, decrypted only for sender/recipient
  - `AlertMessage` — restricted to higher-clearance users
- Messages are persisted to file and loaded on startup

### 📊 Monitoring & Reports
- **Anomaly detection** — scans audit logs for suspicious activity (failed logins, excessive reassignments)
- **Progress reports** — tracks on-time vs. late task completions per user
- **Interactive dashboard** — displays role info, task summary, and performance metrics
- **Global notifications** with three severity levels (Info / Warning / Emergency)

---

## 🏗 Architecture

```
┌─────────────────────────────────────────────────────────┐
│                      Main.cpp                           │
│              (Entry point, menu loop, UI)                │
├──────────┬──────────┬──────────────┬────────────────────┤
│ Users    │ Task     │ Message      │ Utilities          │
│ Module   │ Handler  │ Handler      │ Module             │
│          │          │              │                    │
│ • User   │ • Task   │ • Message    │ • PolicyEngine     │
│ • Junior │ • Task   │   (abstract) │ • AuditLogger      │
│ • Emp.   │   Manager│ • InfoMsg    │ • CredentialMgr    │
│ • Mgr.   │          │ • PrivateMsg │ • PasswordManager  │
│ • Dir.   │          │ • AlertMsg   │ • MFAHandler       │
│ • Exec.  │          │ • MessageMgr │ • AnomalyDetector  │
│          │          │              │ • GlobalNotifier   │
│          │          │              │ • TimeManager      │
└──────────┴──────────┴──────────────┴────────────────────┘
              ▼              ▼               ▼
         users.txt      tasks.txt       inbox.txt
                                        audit.txt
```

---

## 🧬 Class Hierarchy

```
User (Base — clearance 0)
 └── Junior (clearance 1)
      └── Employee (clearance 2)
           └── Manager (clearance 3)
                └── Director (clearance 4)
                     └── Executive (clearance 5)

Message (Abstract Base)
 ├── InfoMessage
 ├── PrivateMessage (encrypted)
 └── AlertMessage
```

Each role class **overrides three virtual functions**:
- `getRoleTitle()` — returns the role name
- `displayDashboard()` — renders role-specific dashboard info
- `canPerformAction(action)` — determines if an action is permitted

---

## 🎓 OOP Concepts Demonstrated

| Concept | Implementation |
|---|---|
| **Multi-Level Inheritance** | 5-level class chain: `User → Junior → Employee → Manager → Director → Executive` |
| **Runtime Polymorphism** | Virtual functions (`getRoleTitle`, `displayDashboard`, `canPerformAction`) called via `User*` base pointers |
| **Abstract Classes & Pure Virtuals** | `Message` is abstract with pure virtual `printFor()`, `serialize()`, `typeCode()` |
| **Operator Overloading** | `<`, `==` on `User` & `Task`; `<<` on `AuditLogger` & `PerformanceReport`; `+=` on `TaskManager` & `PerformanceReport`; `()` functor on `PolicyEngine` & `AnomalyDetector` |
| **Encapsulation** | Private/protected data members with public accessor methods throughout |
| **Composition** | `TaskManager` uses `TimeManager`; `AnomalyDetector` uses `AuditLogger` & `MessageManager` |
| **Friend Functions** | `operator<<` for `AuditLogger`, `PerformanceReport`; `operator<` and `operator==` for `Task` |
| **Linked Lists** | `Task` and `MessageManager::Node` are manually managed linked lists (no STL containers) |
| **File I/O** | Text-based serialization/deserialization for users, tasks, messages, audit logs, and reports |
| **Recursion** | Recursive task expiration (`TimeManager::expireRec`) and delegation traversal (`TaskManager::delegateRec`) |

---

## 🚀 Getting Started

### Prerequisites

- A C++ compiler supporting **C++11** or later (g++, clang++, or MSVC)
- A terminal with **ANSI color support** (most modern terminals)

### Compilation

**Linux / macOS:**
```bash
g++ -std=c++11 -o osim Main.cpp Users.cpp Utilities.cpp TaskHandler.cpp MessageHandler.cpp TimeManager.cpp
```

**Windows (MSVC — Developer Command Prompt):**
```cmd
cl /EHsc /std:c++17 Main.cpp Users.cpp Utilities.cpp TaskHandler.cpp MessageHandler.cpp TimeManager.cpp /Fe:osim.exe
```

**Windows (MinGW / g++):**
```bash
g++ -std=c++11 -o osim.exe Main.cpp Users.cpp Utilities.cpp TaskHandler.cpp MessageHandler.cpp TimeManager.cpp
```

### Run

```bash
./osim          # Linux / macOS
osim.exe        # Windows
```

---

## 👤 Default Users

The system ships with 5 pre-configured demo accounts in `users.txt`:

| Username | Password | Role | Clearance |
|---|---|---|---|
| `junior1` | `junior1` | Junior | 1 |
| `employee1` | `employee1` | Employee | 2 |
| `manager1` | `manager1` | Manager | 3 |
| `director1` | `director1` | Director | 4 |
| `executive1` | `executive1` | Executive | 5 |

> **Note:** Passwords are salted and hashed — the system verifies credentials through `PasswordManager::hashpass()`.

---

## 📖 Usage Guide

After logging in (with MFA verification), the main menu provides **16 options**:

| # | Action | Description |
|---|---|---|
| 1 | **Switch User** | Log out and log in as a different user |
| 2 | **Create Task** | Assign a new task to a user (clearance check enforced) |
| 3 | **Delegate Task** | Re-assign your task to another user (cycle detection active) |
| 4 | **Start Task** | Transition a task from Assigned → In-Progress |
| 5 | **Complete Task** | Mark a task as done (priority ordering enforced) |
| 6 | **List Users** | View all registered users sorted alphabetically |
| 7 | **Add User** | Register a new user (Manager+ required) |
| 8 | **My Tasks** | View tasks assigned to or created by you |
| 9 | **Send Message** | Send an Info, Private, or Alert message |
| 10 | **Inbox** | View received messages |
| 11 | **Progress Report** | View your performance or all reports (Director+) |
| 12 | **Anomaly Report** | View detected anomalies (Manager+ required) |
| 13 | **Clear Anomalies** | Reset the anomaly report (Manager+ required) |
| 14 | **Global Notify** | Broadcast a system-wide notification |
| 15 | **Dashboard** | View your personalized dashboard with stats |
| 16 | **Exit** | Save all data and exit the application |

---

## 📁 Project Structure

```
oop-project-LMS-/
│
├── Main.cpp              # Entry point, menu system, and UI rendering
├── Users.h               # User base class + 5-level role hierarchy declarations
├── Users.cpp             # Role hierarchy implementations with virtual overrides
├── Utilities.h           # Core utility classes (PolicyEngine, AuditLogger, etc.)
├── Utilities.cpp         # Implementations for all utility classes
├── TaskHandler.h         # Task and TaskManager class declarations
├── TaskHandler.cpp       # Task lifecycle, delegation, and persistence logic
├── MessageHandler.h      # Message hierarchy (abstract base + 3 subtypes)
├── MessageHandler.cpp    # Messaging system implementation with encryption
├── TimeManager.h         # TTL expiration manager (recursive traversal)
├── TimeManager.cpp       # TimeManager implementation
│
├── users.txt             # Persistent user credentials (salted + hashed)
├── tasks.txt             # Serialized task data
├── inbox.txt             # Serialized messages
├── audit.txt             # Append-only audit log
├── progressreport.txt    # Per-user task completion statistics
│
├── .gitignore            # Git ignore rules
└── README.md             # This file
```

---

## 💾 Data Files

| File | Purpose | Format |
|---|---|---|
| `users.txt` | Stores user credentials | `username:hash:salt:Role` |
| `tasks.txt` | Persists all tasks | `id\|creator\|assignee\|status\|priority\|createdAt\|ttl\|signature` |
| `inbox.txt` | Stores all messages | `type\|sender\|recipient\|payload` |
| `audit.txt` | Immutable action log | `[timestamp] username ACTION detail` |
| `progressreport.txt` | Performance tracking | `username total onTime late` |
| `otp.txt` | Temporary MFA code | Auto-generated, auto-cleared |
| `anomalyreport.txt` | Detected anomalies | `[timestamp] username ANOMALY_TYPE detail` |

---

## 🛠 Technologies Used

- **Language:** C++ (C++11 standard)
- **Data Structures:** Custom linked lists (no STL containers for core structures)
- **Security:** Salt-based XOR password hashing, XOR message encryption, OTP-based MFA
- **UI:** ANSI escape codes for colors, Unicode box-drawing characters for menus
- **File I/O:** Text-based serialization with `fstream`

---

## 📄 License

This project was developed as a university coursework project for an Object-Oriented Programming course.

---

