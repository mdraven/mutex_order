#pragma once

#include <mutex>
#include <type_traits>

template<class Tag, class Mutex = std::mutex>
class named_mutex : public Mutex {};

template<class T, class... NamedMutexes>
class mutex_order;

// end_element
struct end_element {};

// named_mutex_index
template<class MutexOrder, class NamedMutex>
struct named_mutex_index;

template<class T, class... NamedMutexes, class NamedMutex>
struct named_mutex_index<mutex_order<T, NamedMutexes...>, NamedMutex> :
    std::integral_constant<size_t, named_mutex_index<mutex_order<NamedMutexes...>, NamedMutex>::value + 1> {};

template<class... NamedMutexes, class NamedMutex>
struct named_mutex_index<mutex_order<NamedMutex, NamedMutexes...>, NamedMutex> : std::integral_constant<size_t, 0> {};

template<class NamedMutex>
struct named_mutex_index<mutex_order<NamedMutex>, end_element> : std::integral_constant<size_t, 1> {};

// named_mutex_type
template<class MutexOrder, size_t Index>
struct named_mutex_type;

template<class T, class... NamedMutexes, size_t Index>
struct named_mutex_type<mutex_order<T, NamedMutexes...>, Index>
    : named_mutex_type<mutex_order<NamedMutexes...>, Index-1> {};
 
template<class T, class... NamedMutexes>
struct named_mutex_type<mutex_order<T, NamedMutexes...>, 0> {
   using type = T;
};

template<class T>
struct named_mutex_type<mutex_order<T>, 0> {
   using type = T;
};

template<class T, size_t Index>
struct named_mutex_type<mutex_order<T>, Index> {
   using type = end_element;
};

// mutex_order_slice
template<class MutexOrder, class NamedMutex = end_element>
class mutex_order_slice {
public:
    template<class TNamedMutex>
    mutex_order_slice(mutex_order_slice<MutexOrder, TNamedMutex>) {
        static_assert(named_mutex_index<MutexOrder, NamedMutex>::value >= named_mutex_index<MutexOrder, TNamedMutex>::value, "Incorrect type");
    }

    template<class TNamedMutex, class Handler>
    auto lock(TNamedMutex &mut, Handler &&handler) {
        static_assert(named_mutex_index<MutexOrder, NamedMutex>::value >= named_mutex_index<MutexOrder, TNamedMutex>::value, "Incorrect type");
        std::lock_guard<TNamedMutex> locker(mut);
        using Type = typename named_mutex_type<MutexOrder, named_mutex_index<MutexOrder, TNamedMutex>::value + 1>::type;
        return handler(mutex_order_slice<MutexOrder, Type>());
    }

private:
    template<class, class...>
    friend class mutex_order;

    template<class, class>
    friend class mutex_order_slice;

    mutex_order_slice() = default;
};

template<class MutexOrder>
class mutex_order_slice<MutexOrder, end_element> {
public:
    template<class TNamedMutex>
    mutex_order_slice(mutex_order_slice<MutexOrder, TNamedMutex>) {}
};

template<class T, class... NamedMutexes>
class mutex_order {
public:
    static mutex_order_slice<mutex_order<T, NamedMutexes...>, T> get() {
        return {};
    }
};