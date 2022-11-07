
#include <array>
#include <cassert>
#include <functional>
#include <map>

#include "mutex_order.hpp"

using AMutex  = named_mutex<struct a_mutex>;
using BMutex  = named_mutex<struct b_mutex>;
using CMutex  = named_mutex<struct c_mutex>;

using MapMutex = named_mutex<struct map_mutex>;
using MapElemMutex = named_mutex<struct map_elem_mutex>;

using MutexOrder = mutex_order<AMutex, MapMutex, BMutex, MapElemMutex, CMutex>;

class MapWithElems {
public:
    int get(int key, mutex_order_slice<MutexOrder, MapMutex> mo_slice) {
        using std::placeholders::_1;
        return mo_slice.lock(_mut, std::bind(&MapWithElems::get_from_map, this, key, _1));
    }

    template<class Handler>
    int get_then(int key, mutex_order_slice<MutexOrder, MapMutex> mo_slice, Handler &&handler) {
        return mo_slice.lock(_mut, [&](mutex_order_slice<MutexOrder, BMutex> mo_slice) {
            int value = get_from_map(key, mo_slice);
            value += handler(mo_slice);
            return value;
        });
    }

private:
    struct Elem {
        MapElemMutex _mut;
        int _value = 0;
    };

    MapMutex _mut;
    std::map<int, Elem> _map;

    // Этот метод лежит в там же классе, что и get. Но на практике это могут быть методы разных объектов.
    int get_from_map(int key, mutex_order_slice<MutexOrder, MapElemMutex> mo_slice) {
        auto &elem = _map[key];
        auto value = mo_slice.lock(elem._mut, [&elem](mutex_order_slice<MutexOrder>) -> int {
            return elem._value;
        });
        return value;
    }
};

int main() {
    MutexOrder order;
    MapWithElems map;

    static_assert(named_mutex_index<MutexOrder, end_element>::value == 5, "");
    static_assert(named_mutex_index<MutexOrder, CMutex>::value == 4, "");
    static_assert(std::is_same<named_mutex_type<MutexOrder, 5>::type, end_element>::value, "");
    static_assert(std::is_same<named_mutex_type<MutexOrder, 4>::type, CMutex>::value, "");

    assert(map.get(1, order.get()) == 0);
    assert(map.get_then(1, order.get(), [](mutex_order_slice<MutexOrder, CMutex> mo_slice) {
        return 1;    
    }) == 1);
}
