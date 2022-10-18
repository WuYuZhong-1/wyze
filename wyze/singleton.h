#ifndef _WYZE_SINGLETON_H_
#define _WYZE_SINGLETON_H_

#include <mutex>
#include <memory>

namespace wyze {

template <typename T, class X = void, int N = 0>
class Single {
public:
    static T* GetInstance() {
        static T val;
        return &val;
    }
};

template <class T, class X = void , int N = 0> 
class SinglePtr {
public:
    static std::shared_ptr<T> GetInstance(){
        static std::shared_ptr<T> val(new T);
        return val;
    }
};

}

#endif //_WYZE_SINGLETON_H_