#pragma once

#include <napi.h>


namespace siglabs {
namespace bind {
namespace higgsSNR {

void init(Napi::Env env, Napi::Object& exports, const std::string role);

}
}
}
