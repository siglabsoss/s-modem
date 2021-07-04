#include "discovery.hpp"

#include <sstream>
#include <vector>

using namespace std;

discovery_t::discovery_t() : ip(""), port0(0), port1(0), id(0), name("") {

}


discovery_t::discovery_t(const std::string _ip, const uint32_t _port0, const uint32_t _port1, const uint32_t _id, const std::string _name) : ip(_ip), port0(_port0), port1(_port1), id(_id), name(_name) 
{

}

discovery_t::discovery_t(const std::string stringIn) {

    std::vector<string> commaSeparated(1);
    unsigned int commaCounter = 0;
    for (unsigned int i=0; i<stringIn.size(); i++) {
        if (stringIn.at(i) == ',') {
            commaSeparated.push_back("");
            commaCounter++;
        } else {
            commaSeparated.at(commaCounter) += stringIn[i];
        }
    }

    // for(unsigned int i = 0; i < commaSeparated.size(); i++) {
    //     cout << commaSeparated[i] << endl;
    // }

    // cout << "sizessssssssss" << endl;
    // cout << commaSeparated[0].size() << endl;
    // cout << commaSeparated[1].size() << endl;
    // cout << commaSeparated[2].size() << endl;
    // cout << commaSeparated[3].size() << endl;
    // cout << commaSeparated[4].size() << endl;

    ip = commaSeparated[0];

    if( commaSeparated[1].size() > 0 ) {
        std::stringstream ss0(commaSeparated[1]);
        ss0 >> port0;
    } else {
        port0 = 0;
    }

    if( commaSeparated[2].size() > 0 ) {
        std::stringstream ss1(commaSeparated[2]);
        ss1 >> port1;
    } else {
        port1 = 0;
    }

    if( commaSeparated[3].size() > 0 ) {
        std::stringstream ss2(commaSeparated[3]);
        ss2 >> id;
    } else {
        id = 0; // fixme, throw?
    }

    name = commaSeparated[4];
}


std::string discovery_t::stringify() const {
    std::stringstream buf;
    buf << ip << "," << port0 << "," << port1 << "," << id << "," << name;
    return buf.str();
}


// a hash only uniquely identifies this object based on ip, port0, port1
// it DOES NOT include id, name
size_t discovery_t::hash() const {
    return
    std::hash<std::string>{}(ip) ^
    std::hash<uint32_t>{}(port0) ^
    std::hash<uint32_t>{}(port1);

}

std::string discovery_t::connect_str(const size_t i) const {
    switch(i) {
        case 0:
        return "tcp://" + ip + ":" + to_string(port0);
        case 1:
        return "tcp://" + ip + ":" + to_string(port1);
        default:
        return "";
    }
}