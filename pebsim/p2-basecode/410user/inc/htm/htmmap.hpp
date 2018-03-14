#ifndef _HTM_MAP_H_
#define _HTM_MAP_H_

extern "C" {
#include <assert.h>
#include <stdint.h>
#include <thread.h>
#include <htm.h>
}

#include <new.hpp>
#include <htm/padded.hpp>

#define SUCCESS _XBEGIN_STARTED
typedef unsigned int Result;
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

typedef uint64_t u64;

struct MapInfo {
    unsigned int smallest_bucket_size;
    unsigned int largest_bucket_size;
    unsigned long average_bucket_size;
    unsigned int number_entries;
};

// Abstract class describing the supported operations of our hash map implementations.
template <typename Value>
class Map {
public:
    enum Error { NOT_FOUND, HMSUCCESS, EOM, LOGIC_ERR };
    Error initialize(u64 capacity);
    Error lookup(Value &val);
    Error insert(u64 key, const Value &val);
    Error remove(u64 key);
    MapInfo validate();
};

template <typename Value>
class SepChainMap : public Map<Value> {
    using Error = typename Map<Value>::Error;

    Padded<u64> _num_entries;

    u64 _capacity;
    mutex_t _table_lock;

    struct ChainNode {
        u64 key;
        Value value;
        ChainNode* next;
    };
    ChainNode** table;
    bool _stop;
    static const u64 REGROW_FACTOR = 2;

    // XXX: Not threadsafe, designed for locking the world.
    void _copy_table(ChainNode** newtable, u64 newcap) {

        for (u64 idx = 0; idx < _capacity; ++idx) {
            ChainNode* n    = table[idx];
            ChainNode* next = nullptr;
            while(n != nullptr) {
                u64 newIdx = n->key % newcap;
                // Insert node into new table, at the end.
                ChainNode* m = newtable[newIdx];
                if (m == nullptr) {
                    // Only one in bucket so far.
                    newtable[newIdx] = n;
                } else {
                    // Add to tail.
                    while (m->next != nullptr) {
                        m = m->next;
                    }
                    m->next = n;
                }
                // We change the next pointers, but want to keep iterating.
                next = n->next;
                n->next = nullptr;
                n = next;
            }
            // XXX: We do this to ensure that we abort all other threads inside of
            // a transaction reading the table.
            table[idx] = nullptr;
        }
    }

    Error _regrow() {

        u64 new_capacity = REGROW_FACTOR * _capacity;
        ChainNode** new_table = new ChainNode*[new_capacity];
        if (new_table == nullptr) return Error::LOGIC_ERR;
        // Wait until we can move forward, so the global lock is unlocked.
        while(_stop) { thr_yield(-1); } // made landslide-friendly instead of tight loop

         // Lock ////////////////////////////////
        mutex_lock(&_table_lock);
        _stop = true;

        ChainNode** old_table = table;

        _copy_table(new_table, new_capacity);
        table = new_table;
        _capacity = new_capacity;

        delete old_table;

        _stop = false;
        mutex_unlock(&_table_lock);
        // Unlock //////////////////////////////

        return Error::HMSUCCESS;

    }

    Error _insert(u64 key, const Value &val, ChainNode* newNode) {
        u64 idx = key % _capacity;
        if (table == nullptr) assert(false);
        ChainNode* n = table[idx];

        if (n == nullptr) {
            table[idx]     = newNode;
            return Error::NOT_FOUND;
        }

        // Assumption that n != nullptr.
        while (n->next != nullptr) {
            n = n->next;
            if (n->key == key) {
                // Overwrite value.
                n->value = val;
                // We didn't need it.
                return Error::HMSUCCESS;
            }
        }

        // We have not found key in this chain.
        n->next        = newNode;
        return Error::NOT_FOUND;
    }

    Error _lookup(u64 key, Value& val) {

        u64 idx = key % _capacity;
        ChainNode* n = table[idx];
        while (n != nullptr) {
            if (n->key == key) {
                val = n->value;
                return Error::HMSUCCESS;
            }
            n = n->next;
        }
        return Error::NOT_FOUND;
    }

    Error _remove(u64 key) {

        u64 idx = key % _capacity;
        ChainNode* n = table[idx];
        if (n == nullptr) {
            return Error::NOT_FOUND;
        }
        if (n->key == key && n->next == nullptr) {
            // XXX: TODO: Deleting inside transaction bad.
            // delete n;
            table[idx] = nullptr;
            return Error::HMSUCCESS;
        }
        while (n->next != nullptr) {
            ChainNode* prev = n;
            n = n->next;
            if (n->key == key) {
                prev->next = n->next;
                // XXX: delete n;
                return Error::HMSUCCESS;
            }
        }
        return Error::NOT_FOUND;
    }

    public:
    Error initialize(u64 capacity) {
        _capacity = capacity;
        _num_entries.set(0);
        table = new ChainNode*[capacity];
        for (u64 idx = 0; idx < capacity; ++idx) {
            table[idx] = nullptr;
        }
        mutex_init(&_table_lock);
        _stop = false;
        if (table == nullptr) {
            return Error::LOGIC_ERR;
        }
        return Error::HMSUCCESS;
    }

    Error lookup(u64 key, Value &val) {

        Error err;
        // Wait until we can move forward, so the global lock is unlocked.
        while(_stop);

        Result status = _xbegin();
        if (status == SUCCESS) {
            if (_stop) {
                _xabort(0);
            }
            err = _lookup(key, val);
            _xend();
        } else {
            mutex_lock(&_table_lock);
            _stop = true;

            err = _lookup(key, val);

            _stop = false;
            mutex_unlock(&_table_lock);
        }

        return err;

    }

    Error insert(u64 key, const Value &val) {

        // Preallocate node to avoid unnecessary aborts.
        ChainNode* newNode = new ChainNode;
        if (newNode == nullptr) return Error::LOGIC_ERR;
        newNode->key   = key;
        newNode->value = val;
        newNode->next  = nullptr;

        Error err;
        // Wait until we can move forward, so the global lock is unlocked.
        while(_stop) { thr_yield(-1); }

        Result status = _xbegin();
        if (status == SUCCESS) {
            if (_stop) {
                _xabort(0);
            }
            err = _insert(key, val, newNode);
            _xend();
        } else {
            mutex_lock(&_table_lock);
            _stop = true;

            err = _insert(key, val, newNode);

            _stop = false;
            mutex_unlock(&_table_lock);
        }

        if (err == Error::HMSUCCESS) {
            // Must delete the newNode we allocated, since it's not needed.
            delete newNode;
        } else if (err == Error::NOT_FOUND) {
            // In this case we used newNode, but we want the external call to
            // refer to this as HMSUCCESS.
            err = Error::HMSUCCESS;

            u64* nEntries = _num_entries.get();
            __sync_fetch_and_add(nEntries, 1);
        }

        if (*_num_entries.get() > _capacity) {
            _regrow();
        }

        return err;
    }

    Error remove(u64 key) {
        assert(false && "Not ready yet, must delete outside of transaction");
        Error err;
        // Wait until we can move forward, so the global lock is unlocked.
        while(_stop) { thr_yield(-1); }

        Result status = _xbegin();
        if (status == SUCCESS) {
            if (_stop) {
                _xabort(0);
            }
            err = _remove(key);
            _xend();
        } else {
            mutex_lock(&_table_lock);
            _stop = true;

            err = _remove(key);

            _stop = false;
            mutex_unlock(&_table_lock);
        }

        if (err == Error::HMSUCCESS) {
            // u64 expected = _num_entries;
            // Spin while trying to increment the counter.
            // while(!_num_entries.compare_exchange_strong(expected, expected-1));
        }

        return err;

    }

    // XXX: Not threadsafe or transaction safe!
    MapInfo validate() {
        MapInfo info = {0, 0, 0, 0};
        for (unsigned int idx = 0; idx < _capacity; idx++) {
            ChainNode* n = table[idx];
            unsigned int bucket_size = 0;
            while (n != nullptr) {
                bucket_size++;
                n = n->next;
            }
            info.number_entries += bucket_size;
            info.smallest_bucket_size =
                MIN(info.smallest_bucket_size, bucket_size);
            info.largest_bucket_size =
                MAX(info.smallest_bucket_size, bucket_size);
        }
        info.average_bucket_size =
            ((unsigned long) info.number_entries) / ((unsigned long) _capacity);
        return info;
    };
};

// Open addressing hash table implementation.
template <typename Value, u64 (*NextFN)(u64)>
class OpenAddrMap : public Map<Value> {
public:
    using Error = typename Map<Value>::Error;

private:
    enum EntryState { OCCUPIED, EMPTY, TOMB };
    struct Entry {
        u64 key;
        Value value;
        EntryState state;
    };
    Entry *_entries;

    // Capacity of the table
    u64 _capacity;

    // Lock for regrowing.
    static const u64 REGROW_FACTOR;
    mutex_t regrow_lock;

    Error regrow() {
        // Lock everything: this lock ends when lock goes out of scope.
        // std::lock_guard<mutex_t> lock(regrow_lock);
        mutex_lock(&regrow_lock);

        // Create new capacity and array.
        u64 oldCapacity = _capacity;
        _capacity = REGROW_FACTOR * oldCapacity;
        if (_capacity <= oldCapacity) {
          // Too big, we've overflowed.
          mutex_unlock(&regrow_lock);
          return Map<Value>::LOGIC_ERR;
        }

        Entry *oldEntries = _entries;
        _entries = new /* (std::nothrow) */ Entry[_capacity];
        // For each entry in the current table, copy it over.
        for (u64 i = 0; i < oldCapacity; ++i) {
            Entry entry = oldEntries[i];
            EntryState state = entry.state;
            if (state == OCCUPIED) {
                insert(entry.key, entry.value);
            }
        }

        delete[] oldEntries;
          mutex_unlock(&regrow_lock);
        return Map<Value>::HMSUCCESS;
    }

public:
    Error initialize(u64 capacity) {
        REGROW_FACTOR = 2;
        _capacity = capacity;
        _entries = new /* (std::nothrow) */ Entry[capacity];
        if (_entries == nullptr) {
            return Map<Value>::EOM;
        }

        for (u64 i = 0; i < _capacity; ++i) {
            _entries[i].state = EMPTY;
        }
        mutex_init(&regrow_lock);

        return Map<Value>::HMSUCCESS;
    }

    Error insert(u64 key, const Value &val) {
        // TODO: Maybe simply modding isn't the best way to go.
        // Let's try to think of a better hash function, or make it generic.
        u64 idx = key % _capacity;

        for (u64 count = 0; count < _capacity; ++count) {
            EntryState state = _entries[idx].state;
            u64 foundKey = _entries[idx].key;

            // If the entry is free,
            // or if we found the key we are trying to insert,
            // then we can put the new value into this entry.
            if (state != OCCUPIED || foundKey == key) {
                _entries[idx].key = key;
                _entries[idx].value = val;
                _entries[idx].state = OCCUPIED;
                return Map<Value>::HMSUCCESS;
            }

            idx = NextFN(idx) % _capacity;
        }

        // XXX: What happens if the table is full and we add two
        // things at the same time?
        return regrow();
    }

    Error lookup(u64 key, const Value &val) {
        u64 idx = key % _capacity;

        for (u64 count = 0; count < _capacity; ++count) {
            EntryState state = _entries[idx].state;
            u64 foundKey = _entries[idx].key;

            if (state == EMPTY) {
                return Map<Value>::NOT_FOUND;
            }

            if (state == OCCUPIED && foundKey == key) {
                val = _entries[idx].value;
                return Map<Value>::HMSUCCESS;
            }

            idx = NextFN(idx) % _capacity;
        }

        return Map<Value>::NOT_FOUND;
    }

    Error remove(u64 key) {
        u64 idx = key % _capacity;

        for (u64 count = 0; count < _capacity; ++count) {
            EntryState state = _entries[idx].state;
            u64 foundKey = _entries[idx].key;

            if (state == EMPTY) {
                return Map<Value>::NOT_FOUND;
            }

            if (state == OCCUPIED && foundKey == key) {
                _entries[idx].state = TOMB;
                return Map<Value>::HMSUCCESS;
            }

            idx = NextFN(idx) % _capacity;
        }

        return Map<Value>::NOT_FOUND;
    }

    MapInfo validate() {
        MapInfo info = {0, 0, 0, 0};
        return info;
    }
};

u64 linearProbe(u64 i) {
    return i + 1;
}

u64 quadraticProbe(u64 i) {
    // FIXME: This is incorrect: we should add a constant
    // to ensure that we are not looping while only checking a few things.
    // e.g., i == 0 or i == _capacity / 2.
    // (In general, i divides _capacity or i = 0)
    // XXX: I have currently set it to +1, but I think we can do better.
    return i * i + 1;
}

template<typename Value>
using LinearProbingMap = OpenAddrMap<Value, linearProbe>;

template<typename Value>
using QuadraticProbingMap = OpenAddrMap<Value, quadraticProbe>;

#endif
