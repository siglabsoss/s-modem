#include "DispatchMockRpc.hpp"
#include "3rd/json.hpp"
#include "HiggsSoapyDriver.hpp"
#include "EventDsp.hpp"

using namespace std;

DispatchMockRpc::DispatchMockRpc(HiggsSettings* s, HiggsDriver* sp): settings(s), soapy(sp) {
    updateFlags();
}

void DispatchMockRpc::updateFlags() {
    print = GET_PRINT_DISPATCH_MOCK_RPC();
}

void DispatchMockRpc::dispatch(const std::string& str) {
    nlohmann::json j;

    try {
        // in >> j;
        j = nlohmann::json::parse(str);
    } catch(nlohmann::json::parse_error &e) {
        // print error message;
        cout << "DispatchMockRpc failed to parse: " + str << endl;
        return;
    }

    int top_count = 0;
    for (auto& x : j.items()){(void)x;top_count++;}

    if( top_count != 2 ) {
        cout << "ERROR: top_count " << top_count << " which should be 2" << endl;
        return;
    }
    
    int u_val = 0;
     // example for an array
    int i = 0;
    for (auto& x : j.items())
    {
        // std::cout << "key: " << x.key() << ", value: " << x.value() << '\n';
        // for( auto& y : x.items() ) {
        //    cout << "x" << endl;
        // }

        if( i == 0 ) {
            if( x.value()["u"].is_null() ) {
                // cout << "WAS NULL" << endl;
                return;
            }
            u_val = x.value()["u"];
            // cout << x.value()["u"] << endl;
        } else {
            handleRequest(u_val, x.value());
        }

        i++;
    }
}

void DispatchMockRpc::handleRequest(const int u_val, const nlohmann::json &j) {
    switch(u_val) {
        case 2:
            handleRequest_2(u_val, j);     // send event
            break;
        case 1000:
            handleRequest_1000s(u_val, j); // set value in settings object
            break;
        case 0: // default value / unhandled
            cout << "DispatchMockRpc::handleRequest found illegal value " << u_val << endl;
        default:
            break;
    }    
}

void DispatchMockRpc::handleRequest_2(const int u_val, const nlohmann::json &j) {
//     typedef struct {
//     size_t d0;
//     size_t d1;
// } custom_event_t;

    if( j["v1"].is_null() || j["v2"].is_null() ) {
        cout << "Some part of the message was missing, dropping" << endl;
        return;
    }

    const auto val1 =  j["v1"];
    const auto val2 =  j["v2"];

    custom_event_t ev;
    ev.d0 = val1.get<uint32_t>();
    ev.d1 = val2.get<uint32_t>();

    soapy->dspEvents->sendEvent(ev);

    if(print) {
        cout << "made it inside " << j.dump() << "\n";
    }
    // << val1 << " " << val2 << endl;
    // cout 
    // if( soapy->dspEvents->role == "tx" ) {
    //     soapy->dspEvents->sendEvent(ev);
    // }
}

void DispatchMockRpc::handleRequest_1000s(const int u_val, const nlohmann::json &j) {
        if( print ) {
            cout << "handleDashboardRequest_1000s()" << endl;
        }


        if( j["k"].is_null() || j["v"].is_null() || j["t"].is_null() ) {
            cout << "Some part of the message was missing, dropping" << endl;
            return;
        } else {
            const auto key =  j["k"];
            const auto val =  j["v"];
            const auto type = j["t"];

            std::vector<std::string> path_vec;
            for (const auto& x : j["k"].items()) {
                path_vec.push_back(x.value());
            }

            bool type_found = false;

            if(type == "bool") {
                settings->vectorSet(
                    j["v"].get<bool>(),
                    path_vec );

                type_found = true;
            }

            if(type == "int") {
                settings->vectorSet(
                    j["v"].get<int>(),
                    path_vec );
                
                type_found = true;
            }

            if(type == "double") {
                settings->vectorSet(
                    j["v"].get<double>(),
                    path_vec );
                
                type_found = true;
            }

            if(type == "string") {
                settings->vectorSet(
                    j["v"].get<std::string>(),
                    path_vec );
                
                type_found = true;
            }

            if( !type_found ) {
                cout << "Type: " << type << " was not found, dropping" << endl;
            }

            // for(auto x : path_vec) {
            //     cout << "fx: " << x << endl;
            // }

            if( print ) {
                cout << "made it inside " << key << " " << val << " " << type << endl;
                cout << j.dump() << endl;
            }
        }
    }