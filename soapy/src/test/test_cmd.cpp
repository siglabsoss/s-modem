#include <rapidcheck.h>
#include "common/CmdRunner.hpp"


#include <vector>
#include <algorithm>
#include <iostream>
#include <unistd.h> // for usleep

using namespace std;



int main() {
    std::string ret;
    {
        // CmdRunner dut = CmdRunner("/bin/bash -c \"ls\"");
        CmdRunner dut = CmdRunner("ls");
        ret = dut.getOutput();
    }

    // bool expected
    std::string expected ("Makefile");

    // std::size_t found = haystack.find(needle);
    std::size_t found = ret.find(expected);
    if (found!=std::string::npos) {
        found = true;
        std::cout << "first 'needle' found at: " << found << '\n';
    } else {
        found = false;
    }

    // assert(found);



    // assert(false);
    cout << "ret was " << ret.size() << " long" << endl;
    cout << "ret was " << ret << endl;


    if(1)  {
        auto packed = CmdRunner::runOnce("! ls");
        int retval = std::get<0>(packed);
        std::string output = std::get<1>(packed);

        // WHY ?
        // http://pubs.opengroup.org/onlinepubs/009695399/functions/exit.html
        int ret_type2 = retval % 0xFF;


        cout << "----------------------------" << endl;

        cout << "str: " << output << endl;
        cout << "ret value   : " << retval << endl;
        cout << "ret value(2): " << ret_type2 << endl;

        assert( retval != 0);
    }


    if(1)  {
        auto packed = CmdRunner::runOnce("echo \"this is a test command, that should return with \" ; ! ls", false, true, true);
        int retval = std::get<0>(packed);
        std::string output = std::get<1>(packed);

        // WHY ?
        // http://pubs.opengroup.org/onlinepubs/009695399/functions/exit.html
        int ret_type2 = retval % 0xFF;


        cout << "----------------------------" << endl;

        cout << "str: " << output << endl;
        cout << "ret value   : " << retval << endl;
        cout << "ret value(2): " << ret_type2 << endl;

        assert( retval != 0);
    }




    return 0;
}

