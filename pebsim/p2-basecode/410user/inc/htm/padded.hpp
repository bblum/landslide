#ifndef __PADDED__
#define __PADDED__

#include <stdint.h>
// #include <type_traits>
#include <stdio.h>

#define CACHE_LINE_SIZE 64

typedef uint64_t u64;

template <typename T>
class Padded {
    // XXX: We allocated a new CACHE_LINE_SIZE and it works. I don't know why. - Nick
    // Rounds up to the nearest CACHE_LINE_SIZE, must add an extra CACHE_
    static const u64 size = CACHE_LINE_SIZE +
        ((CACHE_LINE_SIZE + sizeof(T) - 1) / CACHE_LINE_SIZE)
        * CACHE_LINE_SIZE;

    // for landslide, who cares about padding
    // (type_traits headers are huge anyway and i don't wanna import them)
    // using as_t = typename std::aligned_storage<size, CACHE_LINE_SIZE>::type;
    T data;

    public:
        Padded() {
        }
        Padded(T value) {
            set(value);
        }
        ~Padded() {
        }
        T* get() {
            return reinterpret_cast<T*>(&data);
        }

        void set(T value) {
            T* addr = reinterpret_cast<T*>(&data);
            *addr = value;
        }
};

#endif
