#include "driver/HiggsSettings.hpp"

#include <iostream>
#include <sstream>
#include <exception>

//
// allows us just type 'json' to mean this whole thing:
//
using json = nlohmann::json;


using std::cout;
using std::endl;

// argument-less constructor
// HiggsSettings::HiggsSettings() {
    // HiggsSettings(DEFAULT_JSON_PATH);
// }

void HiggsSettings::firstParse() {
    auto it_there = j.find("global");

    // cout << "there " << endl;

    if( it_there == j.end() ) {
        // cout << "value was NOT found" << endl;
     // << it_there << endl;
    } else {
        // cout << "value WAS found" << endl;
    }

    auto prev = *it_there;
    // cout << "prev " << prev << endl;
    auto here2 = prev.find("performance");

    if( here2 == prev.end() ) {
        // cout << "value2 was NOT found" << endl;
     // << here2 << endl;
    } else {
        // cout << "value2 WAS found" << endl;
    }



    // cout << j["global"] << endl;
    // cout << j["global"]["performance"] << endl;
    // cout << j["global"]["performance"]["DEBUG_PRINT_LOOPS"] << endl;

    if(j["global"]["performance"] == NULL) {
        cout << "missing key hive" << endl;
    }


}

HiggsSettings::HiggsSettings(std::string fname, std::string patch) {

    cout << "HiggsSettings() is about to open json file name: " << fname << endl;

    std::ifstream ifs(fname);
    ifs >> j;

    if( patch != "" ) {
        cout << "HiggsSettings() is about to open json patch file name: " << patch << endl;
        std::ifstream ifs_patch(patch);

        nlohmann::json j_patch;
        ifs_patch >> j_patch;

        j.merge_patch(j_patch);
    }

    this->firstParse();
}

// std::string HiggsSettings::get2(std::string key) {
//     auto is_found = j.find(key);

//     // cout << "there " << endl;

//     if( is_found == j.end() ) {
//         // cout << "value was NOT found" << endl;

//         std::string message = "HiggsSettings key '";
//         message += key;
//         message += "' was not found. Aborting";

//         throw std::runtime_error(message.c_str());
        
//         return "";
//      // << it_there << endl;
//     } else {
//         // cout << "value WAS found" << endl;
//         // return is_found.to_string();
//         return *is_found;
//     }
// }



// std::string HiggsSettings::get(std::string key) {
//     std::lock_guard<std::mutex> lock(m);
//     return _map[key];
// }

// size_t HiggsSettings::getInt(std::string key) {
//     size_t result;
//     {
//         std::lock_guard<std::mutex> lock(m);
//         std::stringstream ssettings(_map[key]);
//         ssettings >> result;
//     }
//     return result;
// }
