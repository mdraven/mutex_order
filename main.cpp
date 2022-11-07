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
#include <functional>

#include "mutex_order.hpp"

using AMutex  = named_mutex<struct a_mutex>;
using BMutex  = named_mutex<struct b_mutex>;
using CMutex  = named_mutex<struct c_mutex>;

using MapMutex = named_mutex<struct map_mutex>;
using MapElemMutex = named_mutex<struct map_elem_mutex>;

using MutexOrder = mutex_order<AMutex, MapMutex, BMutex, MapElemMutex, CMutex>;

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
        auto value = mo_slice.lock(elem._mut, [&elem](mutex_order_slice<>) -> int {
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
