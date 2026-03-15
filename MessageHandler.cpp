// ============================================================================
// MessageHandler.cpp — Secure messaging implementation
// Demonstrates: runtime polymorphism via virtual functions, encryption,
//               dynamic memory management, linked list operations
// ============================================================================
#include"MessageHandler.h"
#include<fstream>
#include<iostream>
using namespace std;

// ======================== Message (Base) ========================
Message::Message(User* s, User* r)
    : sender(s), recipient(r), timestamp(Clock::now()) {}
Message::~Message() {}

// ======================== InfoMessage ========================
InfoMessage::InfoMessage(User* s, User* r, const string &txt)
    : Message(s, r), payload(txt) {}

void InfoMessage::printFor(const User* viewer) const {
    char buf[64];
    Time t = timestamp;
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    cout << Color::CYAN << "  [" << buf << "] "
         << Color::WHITE << sender->getUsername() << Color::DIM << " → "
         << Color::WHITE << recipient->getUsername()
         << Color::GREEN << " (INFO): " << Color::RESET
         << payload << "\n";
}

string InfoMessage::serialize() const {
    return sender->getUsername() + "," + recipient->getUsername()
         + ",0," + to_string(timestamp) + "," + payload;
}

MessageType InfoMessage::typeCode() const { return INFO; }

// ======================== PrivateMessage (Encrypted) ========================
// XOR-based symmetric encryption
string PrivateMessage::crypt(const string &in) {
    string out = in;
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] ^= 0xAA;
    }
    return out;
}

// Normal constructor — encrypts the plaintext
PrivateMessage::PrivateMessage(User* s, User* r, const string &txt)
    : Message(s, r), cipher(crypt(txt)) {}

// Raw constructor — used during deserialization (data is already encrypted)
// This fixes the double-encryption bug in the original code
PrivateMessage::PrivateMessage(User* s, User* r, const string &raw, bool isRaw)
    : Message(s, r), cipher(raw) {}

void PrivateMessage::printFor(const User* viewer) const {
    char buf[64];
    Time t = timestamp;
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    if (viewer == recipient) {
        // Only the intended recipient can decrypt
        cout << Color::MAGENTA << "  [" << buf << "] "
             << Color::WHITE << sender->getUsername() << Color::DIM << " → "
             << Color::WHITE << recipient->getUsername()
             << Color::MAGENTA << " (PRIVATE): " << Color::RESET
             << crypt(cipher) << "\n";
    } else {
        cout << Color::DIM << "  [" << buf << "] <encrypted private message>"
             << Color::RESET << "\n";
    }
}

string PrivateMessage::serialize() const {
    return sender->getUsername() + "," + recipient->getUsername()
         + ",1," + to_string(timestamp) + "," + cipher;
}

MessageType PrivateMessage::typeCode() const { return PRIVATE; }

// ======================== AlertMessage ========================
AlertMessage::AlertMessage(User* s, User* r, const string &txt)
    : Message(s, r), payload(txt) {}

void AlertMessage::printFor(const User* viewer) const {
    char buf[64];
    Time t = timestamp;
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    cout << Color::BRED << "  *** ALERT [" << buf << "] ***\n"
         << "  " << payload << Color::RESET << "\n";
}

string AlertMessage::serialize() const {
    return sender->getUsername() + "," + recipient->getUsername()
         + ",2," + to_string(timestamp) + "," + payload;
}

MessageType AlertMessage::typeCode() const { return ALERT; }

// ======================== MessageManager ========================
MessageManager::Node::Node(Message* m, Node* n) : msg(m), next(n) {}

MessageManager::MessageManager(const char* fn) : head(nullptr), inboxFile(fn) {}

MessageManager::~MessageManager() { cleanup(); }

// Add a message to the linked list
void MessageManager::send(Message* m) {
    head = new Node(m, head);
}

// Display all messages for a specific user
void MessageManager::listFor(const User* u) const {
    // Count messages for this user
    int count = 0;
    for (Node* p = head; p; p = p->next) {
        if (p->msg->recipient == u) ++count;
    }
    if (!count) {
        printInfo("Inbox empty.");
        return;
    }

    // Collect into dynamic array for sorting
    Message** arr = new Message*[count];
    int idx = 0;
    for (Node* p = head; p; p = p->next) {
        if (p->msg->recipient == u) arr[idx++] = p->msg;
    }

    // Selection sort by timestamp (newest first)
    for (int i = 0; i < count; ++i) {
        int maxIdx = i;
        for (int j = i + 1; j < count; ++j) {
            if (arr[j]->timestamp > arr[maxIdx]->timestamp)
                maxIdx = j;
        }
        Message* tmp = arr[i]; arr[i] = arr[maxIdx]; arr[maxIdx] = tmp;
    }

    // Display sorted messages using polymorphic printFor()
    cout << Color::BCYAN << "\n  ═══ Inbox (" << count
         << " messages) ═══" << Color::RESET << "\n";
    for (int i = 0; i < count; ++i) {
        arr[i]->printFor(u);  // runtime polymorphism
    }
    cout << Color::BCYAN << "  ═════════════════════\n" << Color::RESET;

    delete[] arr;
}

// Persist messages to inbox file
void MessageManager::save() const {
    ofstream out(inboxFile);
    for (Node* p = head; p; p = p->next) {
        out << p->msg->serialize() << "\n";
    }
}

// Load messages from inbox file, reconstructing correct subclass objects
void MessageManager::load(User** users, int userCount) {
    cleanup();
    ifstream in(inboxFile);
    string line;
    while (getline(in, line)) {
        size_t pos = 0, nxt;
        string parts[5];
        for (int i = 0; i < 4; ++i) {
            nxt = line.find(',', pos);
            if (nxt == string::npos) break;
            parts[i] = line.substr(pos, nxt - pos);
            pos = nxt + 1;
        }
        parts[4] = line.substr(pos);

        // Resolve user pointers
        User* s = nullptr;
        User* r = nullptr;
        for (int i = 0; i < userCount; ++i) {
            if (users[i]->getUsername() == parts[0]) s = users[i];
            if (users[i]->getUsername() == parts[1]) r = users[i];
        }
        if (!s || !r) continue;

        int typeCode = stoi(parts[2]);
        Time ts      = (Time)stoll(parts[3]);
        string payload = parts[4];

        Message* m = nullptr;
        if (typeCode == INFO) {
            m = new InfoMessage(s, r, payload);
        } else if (typeCode == PRIVATE) {
            // Use RAW constructor — data is already encrypted in file
            m = new PrivateMessage(s, r, payload, true);
        } else if (typeCode == ALERT) {
            m = new AlertMessage(s, r, payload);
        }
        if (m) {
            m->timestamp = ts;
            send(m);
        }
    }
}

// Free all message memory
void MessageManager::cleanup() {
    while (head) {
        Node* nxt = head->next;
        delete head->msg;
        delete head;
        head = nxt;
    }
}
