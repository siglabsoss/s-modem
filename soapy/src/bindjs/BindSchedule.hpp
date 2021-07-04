#pragma once

#include <napi.h>


namespace siglabs {
namespace bind {
namespace schedule {

void scheduleSetTimeslots(const Napi::CallbackInfo& info);

void debug(const Napi::CallbackInfo& info);


void init(Napi::Env env, Napi::Object& exports);

}
}
}