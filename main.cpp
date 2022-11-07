// Чтобы не было deadlock'ов мьютексы нужно всегда захватывать в нужном порядке. Чтобы этот порядок был
// контролируем введём 2 сущности.
// 1. уникальный тип для мьютекса, чтобы отличать один от другого:
//   using MapMutex = named_mutex<struct map_mutex>;
//   using MapElemMutex = named_mutex<struct map_elem_mutex>;
// 2. тип который определяет в каком порядке можно захватывать мьютексы:
//   using MutexOrder = mutex_order<MapMutex, MapElemMutex>;
// Для 2 не обязательно чтобы захватывались все мьютексы из списка. И не обязательно чтобы не было дыр при захвате.
// Главное, чтобы они захватывались в приведённом порядке.
// Таким образом MutexOrder может быть один на всю программу и содержать все named_mutex.
//


// TODO: написать про случай с элементами из map в которых есть мьютекс. И о том, что данная штука для этого не применима
// и надо делать постаринке через mutex.lock().

#include <array>
#include <map>
#include <mutex>
#include <functional>

template<class Tag>
class named_mutex : public std::mutex {};

template<class... NamedMutexes>
class mutex_order;

template<class... NamedMutexes>
class mutex_order_slice {
public:
    template<class... SliceNamedMutexes>
    mutex_order_slice(mutex_order_slice<SliceNamedMutexes...>) {
        static_assert(can_cut_all<mutex_order_slice<NamedMutexes...>, mutex_order_slice<SliceNamedMutexes...>>::value, "Incorrect type");
    }

    template<class Mutex, class Handler>
    auto lock(Mutex &mut, Handler &&handler) {
        std::lock_guard<Mutex> locker(mut);
        return handler(shrinkSlice<Mutex>());
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
    typename cut<Mutex, mutex_order_slice<NamedMutexes...>>::type shrinkSlice() {
        return {};
    }

    template<class MutexOrderSlice1, class MutexOrderSlice2>
    struct can_cut_all;

    template<class MutexOrderSlice, class X, class... T>
    struct can_cut_all<mutex_order_slice<X, T...>, MutexOrderSlice> {
        static constexpr bool value = can_cut_all<mutex_order_slice<T...>, typename cut<X, MutexOrderSlice>::type>::value;
    };
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

using MapMutex = named_mutex<struct map_mutex>;
using MapElemMutex = named_mutex<struct map_elem_mutex>;

using MutexOrder = mutex_order<MapMutex, MapElemMutex>;

class MapWithElems {
public:
    int get(int key, mutex_order_slice<MapMutex, MapElemMutex> mo_slice) {
        using std::placeholders::_1;
        using std::placeholders::_2;
        return mo_slice.lock(_mut, std::bind(&MapWithElems::getFromMap, this, key, _1));
    }

private:
    struct Elem {
        MapElemMutex _mut;
        int _value = 0;
    };

    MapMutex _mut;
    std::map<int, Elem> _map;

    // Этот метод лежит в там же классе, что и get. Но на практике это могут быть методы разных объектов.
    int getFromMap(int key, mutex_order_slice<MapElemMutex> mo_slice) {
        auto &elem = _map[key];
        auto value = mo_slice.lock(elem._mut, [&elem](mutex_order_slice<> mo_slice) -> int {
            return elem._value;
        });
        return value;
    }
};

int main() {
    MutexOrder order;
    MapWithElems map;
    return map.get(1, order.get());
}

/*
#include <string>

#include <immer/map.hpp>

int main()
{
    const auto v0 = immer::map<std::string, int>{};
    const auto v1 = v0.set("hello", 42);
    assert(v0["hello"] == 0);
    assert(v1["hello"] == 42);

    const auto v2 = v1.erase("hello");
    assert(*v1.find("hello") == 42);
    assert(!v2.find("hello"));
}
*/