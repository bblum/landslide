#ifndef _AVL_H_
#define _AVL_H_

extern "C" {
#include <assert.h>
#include <htm.h>
#include <stdint.h>
#include <thread.h>
}

// #include <algorithm> // ?
// #include <tuple> // _insert(); edited out
#include <htm/treeinfo.hpp>
#include <new.hpp>

using namespace std;

typedef uint64_t u64;

#define SUCCESS (int)_XBEGIN_STARTED
typedef unsigned int Result;
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

template<typename T>
class AVL {
    struct Node {
        u64 key;
        T val;
        int height;
        Node *left;
        Node *right;
    };

    int nodeHeight(Node *n) {
        return n == nullptr ? 0 : n->height;
    }

    void computeHeight(Node *n) {
        int prev_height = n->height;
        int new_height =  MAX(nodeHeight(n->left), nodeHeight(n->right)) + 1;
        if (prev_height != new_height) {
            n->height = new_height;
        }
        return;
    }

    /*
     *   n               R
     *     \            /
     *      R  ---->   n
     *     /            \
     *    z              z
     */
    Node *rotate_left(Node *n) {
        Node * R =  n->right;
        n->right = R->left;
        R->left = n;
        computeHeight(n);
        computeHeight(R);
        return R;
    }

    /*
     *     n           L
     *    /             \
     *   L    ----->     n
     *    \             /
     *     c           c
     */
    Node *rotate_right(Node *n) {
        Node *L = n->left;
        n->left = L->right;
        L->right = n;
        computeHeight(n);
        computeHeight(L);
        return L;
    }

    Node *rebalance_right(Node *n) {
        if (nodeHeight(n->right) - nodeHeight(n->left) == 2) {
            if (nodeHeight(n->right->right) > nodeHeight(n->right->left)) {
                n = rotate_left(n);
            } else {
                n->right = rotate_right(n->right);
                n = rotate_left(n);
            }
        } else {
            computeHeight(n);
        }
        return n;
    }

    Node *rebalance_left(Node *n) {
        if (nodeHeight(n->left) - nodeHeight(n->right) == 2) {
            if (nodeHeight(n->left->left) > nodeHeight(n->left->right)) {
                n = rotate_right(n);
            } else {
                n->left = rotate_left(n->left);
                n = rotate_right(n);
            }
        } else {
            computeHeight(n);
        }

        return n;
    }

    // tuple<Node *, bool> _insert(Node *cur, Node *n) {
    Node *_insert(Node *cur, Node *n, bool *outp) {
        if (cur == nullptr) {
            *outp = true;
            return n;
            //return tuple<Node *, bool>(n, true);
        }

        bool inserted = false;
        if (cur->key == n->key) {

            cur->val = n->val;

            *outp = false;
            return cur;
            // return tuple<Node *, bool>(cur, false);
        } else if (n->key < cur->key) {
            Node *prev = cur->left;

            Node *new_left;
            // tie(new_left, inserted) = _insert(cur->left, n);
            new_left = _insert(cur->left, n, &inserted);

            if (prev != new_left) {
                cur->left = new_left;
            }

            cur = rebalance_left(cur);
        } else {
            Node *prev = cur->right;
            Node *new_right;
            // tie(new_right, inserted) = _insert(cur->right, n);
            new_right = _insert(cur->right, n, &inserted);

            if (prev != new_right) {
                cur->right = new_right;
            }

            cur = rebalance_right(cur);
        }

        *outp = inserted;
        return cur;
        // return tuple<Node *, bool>(cur, inserted);
    }

    mutex_t _tree_lock;

    Node *_root;

    TreeInfo _validate(Node *n) {
        if (n == nullptr) {
            return TreeInfo(0, 0);
        }

        TreeInfo left = _validate(n->left);
        TreeInfo right = _validate(n->right);

        assert(abs(left.depth - right.depth) <= 1);

        if (n->left != nullptr) {
            assert(n->left->key < n->key);
        }

        if (n->right != nullptr) {
            assert(n->key < n->right->key);
        }

        return TreeInfo(
                MAX(left.depth, right.depth) + 1, left.size + right.size + 1
               );
    }

    bool _lookup(Node *n, u64 key, T *val) {
        if (n == nullptr) {
            return false;
        }

        if (key == n->key) {
            *val = n->val;
            return true;
        } else if (key < n->key) {
            return _lookup(n->left, key, val);
        } else {
            return _lookup(n->right, key, val);
        }
    }

public:

    AVL() : _root(nullptr) {
        mutex_init(&_tree_lock);
    };

    TreeInfo validate() {
        return _validate(_root);
    }

    bool lookup(u64 key, T *val) {

        bool found = false;
        mutex_lock(&_tree_lock);
        found = _lookup(_root, key, val);
        mutex_unlock(&_tree_lock);

        return found;
    }

    void insert(u64 key, T val) {
        Node *n = new Node;
        n->key = key;
        n->val = val;
        n->height = 1;
        n->left = nullptr;
        n->right = nullptr;

        bool inserted = false;

        mutex_lock(&_tree_lock);
        // tie(_root, inserted) = _insert(_root, n);
        _root = _insert(_root, n, &inserted);
        mutex_unlock(&_tree_lock);

        if (!inserted) {
            delete n;
        }

        return;
    }
};

#endif
