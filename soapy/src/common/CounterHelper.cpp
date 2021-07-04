#include "common/CounterHelper.hpp"

CounterHelper::CounterHelper() : counter(0) {

}

int CounterHelper::inc() {
    auto copy = counter;
    counter++;
    return copy;
}