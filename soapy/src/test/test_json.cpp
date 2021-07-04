#include <rapidcheck.h>
// #include "driver/StatusCode.hpp"
#include "3rd/json.hpp"



#include <vector>
#include <algorithm>
#include <iostream>
#include <unistd.h> // for usleep

using namespace std;

#define g_timeout ((double)0.001)

void handle_key(nlohmann::json j) {
    if( !j["$set"].is_null() ) {
        auto unpack = j["$set"];
        if( !unpack["d"].is_null() && !unpack["k"].is_null() ) {
            cout << "Made it CENTER" << endl;
        }
    }
}

int main() {

    if(0) {
    // test_json.cpp

    std::string t1 = "[{\"u\":0},{\"$set\":{\"k\":\"l\",\"d\":\"d\"}}]";
// [{"u":0},{"$set":{"k":"s","d":"u"}}]


        

    // std::string t1 =
    // "{\n\t\"array\": [\n\t\t1,\n\t\t2,\n\t\t3,\n\t\t4\n\t],\n\t\"boolean\": false,\n\t\"null\": null,\n\t\"number\": 42,\n\t\"object\": {},\n\t\"string\": \"Hello world\"\n}";

    nlohmann::json j(nlohmann::json::parse(t1));

    // std::string s = j.dump();
    // cout << "Dump s: " << s << endl;

    // const auto top_count =
    //     std::accumulate(j.begin(), j.end(), std::size_t(0),
    //                                         [=](double lambdaError, uint64_t x) {
    //                                          double diff = 1.0 - (x / ideal);
    //                                          return lambdaError + (diff * diff);
    //                                        });


    int top_count = 0;
    for (auto& x : j.items()){top_count++;}
    cout << "top_count " << top_count << endl;

    if( top_count != 2 ) {
        return 0;
    }

    
    int u_key = 0;
     // example for an array
    int i = 0;
    for (auto& x : j.items())
    {
        std::cout << "key: " << x.key() << ", value: " << x.value() << '\n';
        // for( auto& y : x.items() ) {
        //    cout << "x" << endl;
        // }

        if( i == 0 ) {
            if( x.value()["u"].is_null() ) {
                cout << "WAS NULL" << endl;
                return 0;
            }
            u_key = x.value()["u"];
            // cout << x.value()["u"] << endl;
        } else {
            handle_key(x.value());
        }

        i++;
    }
}

    if(1) {
        std::vector<int> f;

        for(int i = 0; i < 4; i++) {
            f.emplace_back(i);
        }

        for(auto x : f) {
            cout << "fx: " << x << endl;
        }
    }




    return 0;
}







// test_json.cpp

// [{"u":0},{"$set":{"k":"l","d":"d"}}]

// [{"u":0},{"$set":{"k":"s","d":"u"}}]

