#pragma once

#include <functional>
#include <vector>


class NullBufferSplit {
public:
    NullBufferSplit(std::function< void(std::string&) > lambda);

    std::function< void(std::string&) > cap;

    // void debug(void);
    void dispatch(void);

    void addData(const char* const d, const size_t len);
    void addData(const unsigned char* const d, const size_t len);

    std::vector<unsigned char> vec;
};
