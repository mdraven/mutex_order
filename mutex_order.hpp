#pragma once

#include <mutex>
#include <type_traits>

template<class Tag>
class named_mutex : public std::mutex {};

template<class... NamedMutexes>
class mutex_order;

template<class... NamedMutexes>
class mutex_order_slice {
public:
    template<class... T>
    mutex_order_slice(mutex_order_slice<T...>) {
        static_assert(can_cut_all<mutex_order_slice<NamedMutexes...>, mutex_order_slice<T...>>::value, "Incorrect type");
    }

    template<class Mutex, class Handler>
    auto lock(Mutex &mut, Handler &&handler) {
        std::lock_guard<Mutex> locker(mut);
        return handler(shrink_slice<Mutex>());
    }

private:
    template<class...>
    friend class mutex_order;

    template<class...>
    friend class mutex_order_slice;

    mutex_order_slice() = default;

    // Выкидывает всё перед Elem и сам Elem
    template<class Elem, class MutexOrderSlice>
    struct cut;

    template<class Elem, class Head, class... Tail>
    struct cut<Elem, mutex_order_slice<Head, Tail...>> {
        using type = typename cut<Elem, mutex_order_slice<Tail...>>::type;
    };

    template<class Elem, class... Other>
    struct cut<Elem, mutex_order_slice<Elem, Other...>> {
        using type = mutex_order_slice<Other...>;
    };

    template<class Mutex>
    typename cut<Mutex, mutex_order_slice<NamedMutexes...>>::type shrink_slice() {
        return {};
    }

    template<class MutexOrderSlice1, class MutexOrderSlice2>
    struct can_cut_all {
        static constexpr bool value = false;
    };

    template<class X, class... T, class... W>
    struct can_cut_all<mutex_order_slice<X, T...>, mutex_order_slice<W...>> {
        static constexpr bool value = can_cut_all<mutex_order_slice<T...>, typename cut<X, mutex_order_slice<W...>>::type>::value;
    };
    //struct can_cut_all<mutex_order_slice<X, T...>, MutexOrderSlice> {
    //    static constexpr bool value = can_cut_all<mutex_order_slice<T...>, typename cut<X, MutexOrderSlice>::type>::value;
    //};
};

template<>
class mutex_order_slice<> {
public:
    // empty
};

template<class... NamedMutexes>
class mutex_order {
public:
    static mutex_order_slice<NamedMutexes...> get() {
        return {};
    }
};