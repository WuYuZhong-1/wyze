#ifndef _WYZE_NONCOPYBLE_H_
#define _WYZE_NONCOPYBLE_H_


class Noncopyable {
public:
    Noncopyable() = default;
    ~Noncopyable() = default;
    Noncopyable(const Noncopyable&) = delete;
    // Noncopyable(const Noncopyable&&) = delete;
    Noncopyable& operator=(const Noncopyable&) = delete;
};

#endif