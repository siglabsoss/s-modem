#include <rapidcheck.h>
#include "driver/StatusCode.hpp"



#include <vector>
#include <algorithm>
#include <iostream>
#include <unistd.h> // for usleep

using namespace std;

#define g_timeout ((double)0.001)

int main() {

    rc::check("check can add and will go stale using 1ms timeout", [](std::string message) {
        auto dut = StatusCode(false, false, (HiggsDriver*) NULL);

        char a = (char)*rc::gen::inRange('A', 'Z');

        std::string path = "foo.";
        path += a;

        // cout << "Path: " << path << endl;

        char b = (char)*rc::gen::inRange('a', 'z');

        std::string path2 = "bar_none.";
        path2 += b;


        dut.create(path, g_timeout);

        dut.set(path, STATUS_ERROR);

        int error;
        status_code_t found = dut.get(path, &error);
        RC_ASSERT(error == 0);
        auto code0 = std::get<0>(found);

        // cout << code0 << endl;
        RC_ASSERT( code0 == STATUS_ERROR);
        
        unsigned got0 = dut.getCode(path, &error);
        RC_ASSERT(error == 0);
        RC_ASSERT( got0 == STATUS_ERROR);

    //     // sleep for longer than the timeout
        usleep(g_timeout*1E6*3);

        unsigned got1 = dut.getCode(path, &error);

        RC_ASSERT(got1 == STATUS_STALE);

        dut.set(path, STATUS_WARN, message);

        unsigned got2 = dut.getCode(path, &error);
        RC_ASSERT(error == 0);

        unsigned got3 = dut.getCode(path2, &error);
        RC_ASSERT(error != 0);

        status_code_t obj0 = dut.get(path, &error);
        RC_ASSERT(error == 0);

        RC_ASSERT(std::get<1>(obj0) == message);

        usleep(g_timeout*1E6*3);

        status_code_t obj1 = dut.get(path, &error);
        RC_ASSERT(error == 0);

        RC_ASSERT(std::get<0>(obj1) == STATUS_STALE);
    });

    rc::check("0 timeout is ok", [](std::string message) {
        auto dut = StatusCode(false, false, (HiggsDriver*) NULL);

        char a = (char)*rc::gen::inRange('A', 'Z');

        std::string path = "foo.";
        path += a;

        dut.create(path, 0, true);

        dut.set(path, STATUS_OK, "all ok boss");

        usleep(1000);

        int error;

        unsigned got2 = dut.getCode(path, &error);

        RC_ASSERT(got2 == STATUS_OK);

        // cout << dut.getPeriodicPrint();

    });



    return 0;
}

