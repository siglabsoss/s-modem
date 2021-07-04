#pragma once

#include <stdint.h>
#include <string>
#include <map>
#include <mutex>
#include <fstream>
#include <iostream>


#include "common/constants.hpp"
#include "3rd/json.hpp"

typedef std::map<std::string, std::string> settings_map_t;


// #define PRINT_VARARGS_RECURSIVE_DEBUG

class HiggsSettings
{
public:
    // default value here if not specified
    HiggsSettings(std::string path = DEFAULT_JSON_PATH, std::string patch = "");


    // std::string get2(std::string key);
    // std::string get(std::string key);
    // size_t getInt(std::string key);


    //
    // This gets called at the end, once we've crawled down into the json object
    //
    template <typename R>
    R _get3(size_t depth, nlohmann::json &jj) {

        #ifdef PRINT_VARARGS_RECURSIVE_DEBUG
        std::cout << "_get3 terminator depth:" << depth << std::endl;
        #endif

        return jj.get<R>();

    }

    //
    // This gets called over and over, each time with a new string in the peel
    // argument
    // 
    template <typename R, typename... T>
    R _get3(size_t depth, nlohmann::json &jj, std::string peel, T... t) {

        #ifdef PRINT_VARARGS_RECURSIVE_DEBUG
        std::cout << "_get3 peeler depth:" << depth << std::endl;
        std::cout << "     peel:" << peel << std::endl;
        #endif


        auto potential_find = jj.find(peel);


        if( potential_find == jj.end() ) {
            std::cout << "value was NOT found for key: " << peel << std::endl;
            throw std::runtime_error("value not found _get3");
        };

        auto jj_unwrapped = *potential_find;

        // std::cout << "     " << typeid(peel).name() << std::endl;
        // std::cout << "     " << typeid(it_there).name() << std::endl;
        // std::cout << "     " << typeid(jj).name() << std::endl;
        // std::cout << "     " << typeid(prev).name() << std::endl;
        // std::cout << "     " << typeid(*it_there).name() << std::endl;



        return _get3<R>(depth+1, jj_unwrapped, t...);
    }

    // template <typename T>
    template <typename R, typename... T>
    R get(T... t) {
        // take a lock, this gets held for all future recursive calls
        std::lock_guard<std::mutex> lock(m);

        // down the rabbit hole
        return _get3<R>(0, j, t...);
    }










    //
    // This gets called at the end, once we've crawled down into the json object
    //
    template <typename R>
    R _get3WithDefault(R default_value, size_t depth, nlohmann::json &jj) {

        #ifdef PRINT_VARARGS_RECURSIVE_DEBUG
        std::cout << "_get3 terminator depth:" << depth << std::endl;
        #endif

        return jj.get<R>();

    }

    //
    // This gets called over and over, each time with a new string in the peel
    // argument
    // 
    template <typename R, typename... T>
    R _get3WithDefault(R default_value, size_t depth, nlohmann::json &jj, std::string peel, T... t) {

        #ifdef PRINT_VARARGS_RECURSIVE_DEBUG
        std::cout << "_get3 peeler depth:" << depth << std::endl;
        std::cout << "     peel:" << peel << std::endl;
        #endif


        auto potential_find = jj.find(peel);


        if( potential_find == jj.end() ) {
            return default_value;
        }
        
        auto jj_unwrapped = *potential_find;

        // std::cout << "     " << typeid(peel).name() << std::endl;
        // std::cout << "     " << typeid(it_there).name() << std::endl;
        // std::cout << "     " << typeid(jj).name() << std::endl;
        // std::cout << "     " << typeid(prev).name() << std::endl;
        // std::cout << "     " << typeid(*it_there).name() << std::endl;



        return _get3WithDefault<R>(default_value, depth+1, jj_unwrapped, t...);
    }

    template <typename R, typename... T>
    R getWithDefault(R default_value, T... t) {
        // take a lock, this gets held for all future recursive calls
        std::lock_guard<std::mutex> lock(m);

        // // down the rabbit hole
        return _get3WithDefault<R>(default_value, 0, j, t...);
    }

    // same as getWithDefault()
    template <typename R, typename... T>
    R getDefault(R default_value, T... t) {
        // take a lock, this gets held for all future recursive calls
        std::lock_guard<std::mutex> lock(m);

        // // down the rabbit hole
        return _get3WithDefault<R>(default_value, 0, j, t...);
    }

    ///
    /// Takes a vector of strings, representing a path
    /// such as ["exp", "our", "role"]
    /// Diggs into object, and then sets the value
    ///
    template <typename R>
    void vectorSet(R value, const std::vector<std::string> &path) {
        std::lock_guard<std::mutex> lock(m);

        nlohmann::json* jj = &j;

        if( path.size() == 0) {
            return;
        }

        // size minus 1, for direction comparison
        const auto size = path.size()-1;

        unsigned i = 0;
        for( auto peel : path ) {
            const bool final_depth = i == size;
            // std::cout << "peel: " << peel << " " << (int)final_depth << std::endl;
            auto potential_find = jj->find(peel);

            if( !final_depth && potential_find == jj->end() ) {
                // std::cout << "is end()" << std::endl;
                (*jj)[peel] = nullptr; // create a sub object

                potential_find = jj->find(peel); // re-set this value
            }

            if(final_depth) {
                (*jj)[peel] = value;
                return;
            }

            jj = &(*potential_find);
            i++;
        }
    }

    template <typename R>
    std::pair<bool, R> vectorGet(const std::vector<std::string> &path) {
        std::lock_guard<std::mutex> lock(m);

        nlohmann::json* jj = &j;

        if( path.size() == 0) {
            return std::pair<bool, R>(false, R());
        }

        // size minus 1, for direction comparison
        const auto size = path.size()-1;

        unsigned i = 0;
        for( auto peel : path ) {
            const bool final_depth = i == size;
            // std::cout << "peel: " << peel << " " << (int)final_depth << std::endl;
            auto potential_find = jj->find(peel);

            if( potential_find == jj->end() ) {
                // std::cout << "failed to find" << std::endl;
                return std::pair<bool, R>(false, R());
            }

            if(final_depth) {
                auto val = (*jj)[peel].get<R>();
                return std::pair<bool, R>(true, val);
            }

            jj = &(*potential_find);
            i++;
        }

        // std::cout << "hit bottom" << std::endl;

        return std::pair<bool, R>(false, R());
    }

    // could add vectorGetDefaultWasDefault
    template <typename R>
    R vectorGetDefault(R value, const std::vector<std::string> &path) {
        // for( auto x : path ) {
            // std::cout << "p: " << x << " ";
        // }

        auto pair = vectorGet<R>(path);
        bool ok = std::get<0>(pair);

        if( ok ) {
            // std::cout << "value of " << std::get<1>(pair) << std::endl;
            return std::get<1>(pair);
        } else {
            // std::cout << "default value of " << value << std::endl;
            return value;
        }
    }

    // returns true for ok/found
    // returns false for not found but returns the default as well
    template <typename R>
    std::pair<bool, R> vectorGetWasFoundDefault(R value, const std::vector<std::string> &path) {
        auto pair = vectorGet<R>(path);
        bool ok = std::get<0>(pair);

        if( ok ) {
            return pair;
        } else {
            return std::pair<bool, R>(false, value);
        }
    }



private:
    void firstParse();

    // settings_map_t _map;
    std::mutex m;
    nlohmann::json j;
};


/// we add this for supporting the PATH_FOR() macro
/// we use duck typing in c++ because everything is a macro
/// Use this like
/// std::vector<std::string> path = PATH_FOR(GET_DASHBOARD_DO_CONNECT())


class HiggsSettingsDuckType
{
public:
    template <typename T>
    static std::vector<std::string> get(
                     const std::string p0
                    ,const std::string p1 = ""
                    ,const std::string p2 = ""
                    ,const std::string p3 = ""
                    ,const std::string p4 = ""
                    ,const std::string p5 = ""
                    ,const std::string p6 = ""
                    ) {
        std::vector<std::string> ret;
        auto foo = {&p0, &p1, &p2, &p3, &p4, &p5, &p6};
        // auto foo = {p0, p1, p2, p3, p4, p5, p6};
        for( const auto& x : foo ) {
            if(*x != "") {
                ret.emplace_back(*x);
            }
        }
        return ret;
        // return {};
    }

    template <typename T>
    static std::vector<std::string> getDefault(
                     const T v
                    ,const std::string p0
                    ,const std::string p1 = ""
                    ,const std::string p2 = ""
                    ,const std::string p3 = ""
                    ,const std::string p4 = ""
                    ,const std::string p5 = ""
                    ,const std::string p6 = ""
                ) {
        return get<T>(p0,p1,p2,p3,p4,p5,p6);
    }

    template <typename T>
    static std::vector<std::string> getWithDefault(
                     const T v
                    ,const std::string p0
                    ,const std::string p1 = ""
                    ,const std::string p2 = ""
                    ,const std::string p3 = ""
                    ,const std::string p4 = ""
                    ,const std::string p5 = ""
                    ,const std::string p6 = ""
                ) {
        return get<T>(p0,p1,p2,p3,p4,p5,p6);
    }
};







// template<typename T>
// void getSetting(const T &p) {
//     std::cout << "whole thing" << std::endl;
// }


// see also:
// template madness above
//    https://monoinfinito.wordpress.com/series/c11-intro-cool-new-features/