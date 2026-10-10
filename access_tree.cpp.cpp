// ============================================================================
//  Lab Access Verification System  -  TREE version (Binary Search Tree only)
//  Every collection is a binary search tree:
//    researchers  -> BST keyed by researcher ID
//    requests     -> BST keyed by arrival number (smallest key = next request)
//    access log   -> BST keyed by decision number (largest key = newest)
//  No arrays, vectors, lists, queues, stacks, sorting or hashing hold the data.
//
//  Console:  g++ -std=c++17 access_tree.cpp -o tree  &&  ./tree
//  Website:  compiled to WebAssembly by GitHub Actions (see README.md)
// ============================================================================
#include <cctype>
#include <ctime>
#include <iostream>
#include <string>
using namespace std;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define EXPORT extern "C" EMSCRIPTEN_KEEPALIVE
#else
#define EXPORT extern "C"
#endif

// ----------------------------------------------------------- tree node ------
template <typename K, typename V>
struct TNode {
    K key;
    V val;
    TNode* left;
    TNode* right;
    TNode(const K& k, const V& v) : key(k), val(v), left(nullptr), right(nullptr) {}
};

// ------------------------------------------------- binary search tree -------
template <typename K, typename V>
class BST {
public:
    typedef TNode<K, V> N;
    N* root = nullptr;
    int size = 0;

    BST() {}
    BST(const BST&) = delete;
    BST& operator=(const BST&) = delete;
    ~BST() { clear(root); }

    // insert: O(h). Returns false if the key already exists.
    bool insert(const K& k, const V& v) {
        N** p = &root;
        while (*p) {
            if (k == (*p)->key) return false;
            p = (k < (*p)->key) ? &(*p)->left : &(*p)->right;
        }
        *p = new N(k, v);
        size++;
        return true;
    }

    // search: O(h). visit(key) is called for every node on the search path.
    template <typename F>
    V* findVisit(const K& k, F visit) {
        N* c = root;
        while (c) {
            visit(c->key);
            if (k == c->key) return &c->val;
            c = (k < c->key) ? c->left : c->right;
        }
        return nullptr;
    }
    V* find(const K& k) { return findVisit(k, [](const K&) {}); }

    // delete: O(h). Handles 0, 1 and 2 children (inorder successor).
    bool remove(const K& k) {
        N** p = &root;
        while (*p && !((*p)->key == k)) p = (k < (*p)->key) ? &(*p)->left : &(*p)->right;
        if (!*p) return false;
        N* n = *p;
        if (!n->left) *p = n->right;                       // 0 or 1 child
        else if (!n->right) *p = n->left;                  // 1 child
        else {                                             // 2 children
            N** s = &n->right;
            while ((*s)->left) s = &(*s)->left;            // inorder successor
            N* succ = *s;
            *s = succ->right;
            succ->left = n->left;
            succ->right = n->right;
            *p = succ;
        }
        delete n;
        size--;
        return true;
    }

    // remove the smallest key (leftmost node): O(h)
    bool popMin(V& out) {
        N** p = &root;
        while (*p && (*p)->left) p = &(*p)->left;
        if (!*p) return false;
        N* n = *p;
        out = n->val;
        *p = n->right;
        delete n;
        size--;
        return true;
    }

    int height() const { return height(root); }            // empty tree = 0

    template <typename F> void inorder(F f)    { in(root, f); }
    template <typename F> void preorder(F f)   { pre(root, f); }
    template <typename F> void postorder(F f)  { post(root, f); }
    template <typename F> void revInorder(F f) { rev(root, f); }   // largest first

private:
    static int height(const N* n) {
        if (!n) return 0;
        int l = height(n->left), r = height(n->right);
        return 1 + (l > r ? l : r);
    }
    static void clear(N* n) { if (n) { clear(n->left); clear(n->right); delete n; } }
    template <typename F> static void in(N* n, F& f)   { if (n) { in(n->left, f); f(n->key, n->val); in(n->right, f); } }
    template <typename F> static void pre(N* n, F& f)  { if (n) { f(n->key, n->val); pre(n->left, f); pre(n->right, f); } }
    template <typename F> static void post(N* n, F& f) { if (n) { post(n->left, f); post(n->right, f); f(n->key, n->val); } }
    template <typename F> static void rev(N* n, F& f)  { if (n) { rev(n->right, f); f(n->key, n->val); rev(n->left, f); } }
};

// ------------------------------------------------------------ records -------
struct Researcher { string id, name, zone; bool active, revoked; };
struct Request    { string id, zone; };
struct Entry      { string id, name, zone, reason, time; bool ok; };

const int PASS = 0, FAIL = 1, SKIP = 2;
struct Result {
    bool empty = true, ok = false, found = false, active = false, revoked = false;
    string id, name, zone, reqZone, reason, path;
    int cmp = 0, c1 = SKIP, c2 = SKIP, c3 = SKIP, c4 = SKIP, c5 = SKIP;
};

// -------------------------------------------------------------- state -------
static BST<string, Researcher> researchers;   // keyed by ID
static BST<long, Request>      requests;      // keyed by arrival number
static BST<long, Entry>        logTree;       // keyed by decision number
static long reqSeq = 0, logSeq = 0;
static bool lockdown = false, started = false;
static int total = 0, granted = 0, denied = 0;

// ------------------------------------------------------------ helpers -------
static string upper(string s) {
    for (size_t i = 0; i < s.size(); i++) s[i] = (char)toupper((unsigned char)s[i]);
    return s;
}
static string trim(const string& s) {
    size_t a = 0, b = s.size();
    while (a < b && isspace((unsigned char)s[a])) a++;
    while (b > a && isspace((unsigned char)s[b - 1])) b--;
    return s.substr(a, b - a);
}
static bool validZone(const string& z) { return z == "LAB1" || z == "LAB2" || z == "LAB3"; }
static bool validId(const string& s) {
    if (s.empty() || s.size() > 12) return false;
    for (size_t i = 0; i < s.size(); i++) if (!isalnum((unsigned char)s[i])) return false;
    return true;
}
static string jq(const string& s) {
    string o = "\"";
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if (c == '"') o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if ((unsigned char)c < 0x20) o += ' ';
        else o += c;
    }
    return o + "\"";
}
static string jb(bool v) { return v ? "true" : "false"; }

// search the researcher tree, remembering the path of IDs visited
static Researcher* lookup(const string& id, string& path, int& cmp) {
    path = ""; cmp = 0;
    return researchers.findVisit(id, [&](const string& k) {
        path += (path.empty() ? "" : ",") + jq(k);
        cmp++;
    });
}

static int step(bool condition, const char* failReason, string& reason) {
    if (!reason.empty()) return SKIP;
    if (!condition) { reason = failReason; return FAIL; }
    return PASS;
}

static Result processNext(const string& when) {
    Result res;
    Request req;
    if (!requests.popMin(req)) return res;        // smallest key = oldest request
    res.empty = false;
    res.id = req.id;
    res.reqZone = req.zone;

    Researcher* r = lookup(req.id, res.path, res.cmp);
    res.found = (r != nullptr);
    res.name = r ? r->name : "Unknown";
    if (r) { res.zone = r->zone; res.active = r->active; res.revoked = r->revoked; }

    string reason;
    res.c1 = step(r != nullptr,             "Unknown researcher ID", reason);
    res.c2 = step(r && r->zone == req.zone, "Zone not authorized",   reason);
    res.c3 = step(r && r->active,           "Permission inactive",   reason);
    res.c4 = step(r && !r->revoked,         "Access revoked",        reason);
    res.c5 = step(!lockdown,                "Emergency lockdown",    reason);

    res.ok = reason.empty();
    res.reason = res.ok ? "All checks passed" : reason;
    total++;
    if (res.ok) granted++; else denied++;
    logTree.insert(++logSeq, Entry{req.id, res.name, req.zone, res.reason, when, res.ok});
    return res;
}

static void seed() {
    if (started) return;
    started = true;
    // insertion order decides the shape of the tree
    researchers.insert("R104", {"R104", "Kiran Shah",    "LAB1", true, true});    // revoked
    researchers.insert("R102", {"R102", "Meera Nair",    "LAB2", true, false});
    researchers.insert("R106", {"R106", "Dev Patel",     "LAB3", true, false});
    researchers.insert("R101", {"R101", "Dr. Anika Rao", "LAB1", true, false});
    researchers.insert("R103", {"R103", "Arjun Das",     "LAB3", true, false});
    researchers.insert("R105", {"R105", "Farah Khan",    "LAB2", true, false});
}

// ====================================================================
//  Functions called by the web page (WebAssembly) - results are JSON
// ====================================================================
static string out;

EXPORT void av_init() { seed(); }

EXPORT int av_request(const char* id, const char* zone) {
    requests.insert(++reqSeq, Request{upper(trim(id)), upper(trim(zone))});
    return requests.size;
}
EXPORT int av_queue_size() { return requests.size; }

EXPORT const char* av_lookup(const char* id) {
    string path; int cmp;
    Researcher* r = lookup(upper(trim(id)), path, cmp);
    out = string("{\"found\":") + jb(r != nullptr);
    if (r) out += ",\"name\":" + jq(r->name) + ",\"zone\":" + jq(r->zone) +
                  ",\"active\":" + jb(r->active) + ",\"revoked\":" + jb(r->revoked);
    out += ",\"path\":[" + path + "],\"cmp\":" + to_string(cmp) + "}";
    return out.c_str();
}

EXPORT const char* av_process_next(const char* when) {
    Result r = processNext(when);
    if (r.empty) { out = "{\"empty\":true}"; return out.c_str(); }
    out = "{\"empty\":false,\"ok\":" + jb(r.ok) + ",\"found\":" + jb(r.found) +
          ",\"id\":" + jq(r.id) + ",\"name\":" + jq(r.name) + ",\"zone\":" + jq(r.zone) +
          ",\"reqZone\":" + jq(r.reqZone) + ",\"active\":" + jb(r.active) +
          ",\"revoked\":" + jb(r.revoked) + ",\"reason\":" + jq(r.reason) +
          ",\"path\":[" + r.path + "],\"cmp\":" + to_string(r.cmp) +
          ",\"checks\":[" + to_string(r.c1) + "," + to_string(r.c2) + "," + to_string(r.c3) +
          "," + to_string(r.c4) + "," + to_string(r.c5) + "]}";
    return out.c_str();
}

EXPORT int av_toggle_lock() { lockdown = !lockdown; return lockdown ? 1 : 0; }

// 0 = added, 1 = missing field, 2 = duplicate ID, 3 = invalid ID or zone
EXPORT int av_add(const char* id, const char* name, const char* zone) {
    string i = upper(trim(id)), n = trim(name), z = upper(trim(zone));
    if (i.empty() || n.empty()) return 1;
    if (!validId(i) || !validZone(z)) return 3;
    return researchers.insert(i, Researcher{i, n, z, true, false}) ? 0 : 2;
}
EXPORT void av_revoke(const char* id) {
    Researcher* r = researchers.find(upper(trim(id)));
    if (r) r->revoked = !r->revoked;
}
EXPORT void av_perm(const char* id) {
    Researcher* r = researchers.find(upper(trim(id)));
    if (r) r->active = !r->active;
}
EXPORT void av_remove(const char* id) { researchers.remove(upper(trim(id))); }

// the researcher tree as nested JSON: {i,n,a,v,l,r}
static string treeJson(TNode<string, Researcher>* n) {
    if (!n) return "null";
    return "{\"i\":" + jq(n->key) + ",\"n\":" + jq(n->val.name) + ",\"a\":" + jb(n->val.active) +
           ",\"v\":" + jb(n->val.revoked) + ",\"l\":" + treeJson(n->left) +
           ",\"r\":" + treeJson(n->right) + "}";
}
EXPORT const char* av_tree() {
    out = treeJson(researchers.root);
    return out.c_str();
}

EXPORT const char* av_state() {
    out = "{\"lock\":" + jb(lockdown) + ",\"total\":" + to_string(total) +
          ",\"granted\":" + to_string(granted) + ",\"denied\":" + to_string(denied) +
          ",\"qsize\":" + to_string(requests.size) + ",\"size\":" + to_string(researchers.size) +
          ",\"height\":" + to_string(researchers.height()) +
          ",\"reqHeight\":" + to_string(requests.height()) +
          ",\"logHeight\":" + to_string(logTree.height()) + ",\"queue\":[";
    bool first = true;
    requests.inorder([&](const long&, const Request& q) {
        out += string(first ? "" : ",") + "{\"id\":" + jq(q.id) + ",\"zone\":" + jq(q.zone) + "}";
        first = false;
    });
    out += "],\"researchers\":[";
    first = true;
    researchers.inorder([&](const string&, const Researcher& r) {
        out += string(first ? "" : ",") + "{\"id\":" + jq(r.id) + ",\"name\":" + jq(r.name) +
               ",\"zone\":" + jq(r.zone) + ",\"active\":" + jb(r.active) +
               ",\"revoked\":" + jb(r.revoked) + "}";
        first = false;
    });
    out += "],\"in\":[";
    first = true;
    researchers.inorder([&](const string& k, const Researcher&) { out += string(first ? "" : ",") + jq(k); first = false; });
    out += "],\"pre\":[";
    first = true;
    researchers.preorder([&](const string& k, const Researcher&) { out += string(first ? "" : ",") + jq(k); first = false; });
    out += "],\"post\":[";
    first = true;
    researchers.postorder([&](const string& k, const Researcher&) { out += string(first ? "" : ",") + jq(k); first = false; });
    out += "],\"log\":[";
    first = true;
    int shown = 0;
    logTree.revInorder([&](const long&, const Entry& e) {          // newest first
        if (shown++ >= 12) return;
        out += string(first ? "" : ",") + "{\"id\":" + jq(e.id) + ",\"name\":" + jq(e.name) +
               ",\"zone\":" + jq(e.zone) + ",\"ok\":" + jb(e.ok) + ",\"reason\":" + jq(e.reason) +
               ",\"time\":" + jq(e.time) + "}";
        first = false;
    });
    out += "]}";
    return out.c_str();
}

// ====================================================================
#ifdef __EMSCRIPTEN__
int main() { seed(); return 0; }
#else
static string nowStr() {
    char buf[16];
    time_t t = time(nullptr);
    strftime(buf, sizeof buf, "%H:%M:%S", localtime(&t));
    return buf;
}
static string ask(const string& prompt) { cout << prompt; string s; getline(cin, s); return trim(s); }
static const char* mark(int c) { return c == PASS ? "Passed" : c == FAIL ? "FAILED" : "Skipped"; }

static void printTree(TNode<string, Researcher>* n, int depth) {   // root at left, rotated view
    if (!n) return;
    printTree(n->right, depth + 1);
    cout << string(depth * 6, ' ') << n->key << (n->val.revoked ? "*" : "") << "\n";
    printTree(n->left, depth + 1);
}
static void doProcess() {
    Result r = processNext(nowStr());
    if (r.empty) { cout << "No pending requests.\n"; return; }
    cout << "Request " << r.id << " for " << r.reqZone << "   (tree search: " << r.cmp << " comparisons)\n"
         << "  Authentication : " << mark(r.c1) << "\n  Authorization  : " << mark(r.c2)
         << "\n  Permission     : " << mark(r.c3) << "\n  Revocation     : " << mark(r.c4)
         << "\n  Lockdown       : " << mark(r.c5) << "\n"
         << (r.ok ? "  ACCESS GRANTED to " + r.reqZone + " for " + r.name
                  : "  ACCESS DENIED: " + r.reason) << "\n";
}
static void traversals() {
    cout << "Inorder   (sorted): "; researchers.inorder([](const string& k, const Researcher&) { cout << k << " "; });
    cout << "\nPreorder          : "; researchers.preorder([](const string& k, const Researcher&) { cout << k << " "; });
    cout << "\nPostorder         : "; researchers.postorder([](const string& k, const Researcher&) { cout << k << " "; });
    cout << "\nSize " << researchers.size << ", height " << researchers.height() << "\n";
}

int main() {
    seed();
    while (true) {
        cout << "\n=== Lab Access Verification (TREE) - " << (lockdown ? "LOCKDOWN ACTIVE" : "SYSTEM ONLINE") << " ===\n"
             << " 1 Send request      2 Process next     3 Process all     4 Show researcher tree\n"
             << " 5 Traversals        6 Add researcher   7 Revoke/restore  8 Activate/deactivate\n"
             << " 9 Remove researcher 10 Lockdown        11 Access log      0 Exit\n";
        string c = ask("Choice: ");
        if (c == "0") break;
        else if (c == "1") { string i = ask("Researcher ID: "), z = ask("Laboratory (LAB1/LAB2/LAB3): ");
                             if (i.empty() || !validZone(upper(z))) cout << "Enter an ID and LAB1, LAB2 or LAB3.\n";
                             else cout << "Queued. " << av_request(i.c_str(), z.c_str()) << " pending.\n"; }
        else if (c == "2") doProcess();
        else if (c == "3") { while (requests.size) doProcess(); }
        else if (c == "4") { printTree(researchers.root, 0); cout << "(* = revoked)\n"; }
        else if (c == "5") traversals();
        else if (c == "6") { string i = ask("ID: "), n = ask("Name: "), z = ask("Zone: ");
                             int rc = av_add(i.c_str(), n.c_str(), z.c_str());
                             cout << (rc == 0 ? "Researcher inserted into the tree." : rc == 1 ? "Enter both an ID and a name."
                                      : rc == 2 ? "That ID already exists." : "Invalid ID or zone.") << "\n"; }
        else if (c == "7") { av_revoke(ask("ID: ").c_str()); printTree(researchers.root, 0); }
        else if (c == "8") { av_perm(ask("ID: ").c_str()); printTree(researchers.root, 0); }
        else if (c == "9") { av_remove(ask("ID: ").c_str()); printTree(researchers.root, 0); }
        else if (c == "10") cout << (av_toggle_lock() ? "Lockdown enabled.\n" : "Lockdown disabled.\n");
        else if (c == "11") { int k = 0; logTree.revInorder([&](const long&, const Entry& e) {
                                if (k++ < 12) cout << e.time << "  " << e.id << "  " << e.zone << "  "
                                                   << (e.ok ? "Granted" : "Denied") << "  (" << e.reason << ")\n"; });
                              if (!k) cout << "nothing yet\n"; }
        else cout << "Invalid choice.\n";
    }
    return 0;
}
#endif
