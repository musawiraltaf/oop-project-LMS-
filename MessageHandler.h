// ============================================================================
// MessageHandler.h — Secure messaging system
// Types: INFO, PRIVATE (encrypted), ALERT (restricted)
// Demonstrates: runtime polymorphism via virtual functions, base class ptrs
// ============================================================================
#pragma once
#include<string>
#include"Users.h"
#include"Utilities.h"
using namespace std;

// ======================== Message Type Enum ========================
enum MessageType { INFO = 0, PRIVATE = 1, ALERT = 2 };

// ======================== Abstract Base: Message ========================
class Message {
public:
    User* sender;
    User* recipient;
    Time  timestamp;

    Message(User* s, User* r);
    virtual ~Message();

    // Pure virtual functions — runtime polymorphism
    virtual void printFor(const User* viewer) const = 0;
    virtual string serialize() const = 0;
    virtual MessageType typeCode() const = 0;
};

// ======================== InfoMessage ========================
class InfoMessage : public Message {
    string payload;
public:
    InfoMessage(User* s, User* r, const string &txt);
    void printFor(const User* viewer) const override;
    string serialize() const override;
    MessageType typeCode() const override;
};

// ======================== PrivateMessage (Encrypted) ========================
class PrivateMessage : public Message {
    string cipher;
protected:
    static string crypt(const string &in);
public:
    // Normal constructor — encrypts the plaintext
    PrivateMessage(User* s, User* r, const string &txt);
    // Raw constructor — payload is already encrypted (used for deserialization)
    PrivateMessage(User* s, User* r, const string &raw, bool isRaw);

    void printFor(const User* viewer) const override;
    string serialize() const override;
    MessageType typeCode() const override;
};

// ======================== AlertMessage ========================
class AlertMessage : public Message {
    string payload;
public:
    AlertMessage(User* s, User* r, const string &txt);
    void printFor(const User* viewer) const override;
    string serialize() const override;
    MessageType typeCode() const override;
};

// ======================== MessageManager ========================
class MessageManager {
    struct Node {
        Message* msg;
        Node* next;
        Node(Message* m, Node* n = nullptr);
    };
    Node* head;
    const char* inboxFile;
public:
    MessageManager(const char* filename = "inbox.txt");
    ~MessageManager();
    void send(Message* m);
    void listFor(const User* u) const;
    void save() const;
    void load(User** users, int userCount);
    void cleanup();
};
