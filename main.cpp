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
class named_mutex {};

template<class... Tag>
class mutex_order {};

template<class MutexOrder, class NamedMutex = void>
class mutex_order_slice {
public:
    template<class Mutex, class Handler>
    auto lock(Mutex &mut, Handler &&handler) -> int/*TODO: выводить тип из Handler*/ {
        std::lock_guard<Mutex> locker(mut);
        return handler(shrinkSlice<Mutex>());
    }
private:
    template<class Mutex>
    auto shrinkSlice() -> mutex_order_slice<MutexOrder, >;
};

template<class MutexOrder>
class mutex_order_slice<MutexOrder, void> {
public:
    // empty
};

using MapMutex = named_mutex<struct map_mutex>;
using MapElemMutex = named_mutex<struct map_elem_mutex>;

using MutexOrder = mutex_order<MapMutex, MapElemMutex>;

class MapWithElems {
public:
    int get(int key, mutex_order_slice<MutexOrder, MapMutex> mo_slice) {
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
    int getFromMap(int key, mutex_order_slice<MutexOrder, MapElemMutex> mo_slice) {
        auto &elem = _map[key];
        auto value = mo_slice.lock(elem._mut, [&elem](mutex_order_slice<MutexOrder> mo_slice) -> int {
            return elem._value;
        });
        return value;
    }
};

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