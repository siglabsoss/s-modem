#include <napi.h>

#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

using namespace Napi;

// #define CTJS_PRINT

template <typename T>
class CToJavascript : public Napi::AsyncWorker {
    private:
        std::mutex& mut;
        std::vector<T>& work;
        std::condition_variable& outputReadyCondition;
        std::atomic<bool> requestShutdown;
        std::function<void()> afterCompleteCallback;

    public:
        CToJavascript(Function& callback, std::mutex& m, std::vector<T>& w, std::condition_variable& c, std::function<void()> complete)
        : AsyncWorker(callback), mut(m), work(w), outputReadyCondition(c), requestShutdown(false), afterCompleteCallback(complete){}

#ifdef CTJS_PRINT
        ~CToJavascript() {
            std::cout << "~CToJavascript()" << std::endl;
        }
#endif
    // This code will be executed on the worker thread
    void Execute() {
        // std::cout << "Execute()" << std::endl;
        std::unique_lock<std::mutex> lk(mut);
        outputReadyCondition.wait(lk, [this]{return (work.size() > 0)||requestShutdown;});
        lk.unlock();
        // std::cout << "Execute() finished" << std::endl;
        
    }

    // For each templatized version of CToJavascript we use
    // we need to make a dispatchType() for that type

    void dispatchType(const uint32_t& word) {
        napi_value val;

        napi_create_uint32(Env(), word, &val);

        Callback().Call({val});
    }

    void dispatchType(const std::vector<uint32_t>& vec) {
        Napi::Env env = Env();
        napi_value val;

        auto status = napi_create_array_with_length(env, vec.size(), &val);
        (void)status;

        unsigned i = 0;
        for(auto w : vec) {
            status = napi_set_element(env, val, i, Napi::Number::New(env,w));
            i++;
        }

        Callback().Call({val});
    }

    void OnOK() {
#ifdef CTJS_PRINT
        std::cout << "OnOK()" << std::endl;
#endif
        // echo = "fooooo";

        HandleScope scope(Env());

        std::vector<T> workCopy; // on stack

        {
            std::lock_guard<std::mutex> lk(mut);
            // take lock, copy to the stack, and empty the shared one
            workCopy = work;
            work.resize(0);
        }

        for(const auto &theT : workCopy) {
            // Call overloaded function
            dispatchType(theT);
        }

        afterCompleteCallback();
    }

    // private:
    //     std::string echo;
};