#include <napi.h>

namespace siglabs {
namespace bind {
namespace data {

napi_value getRxData(const Napi::CallbackInfo& info);
void queueTxData(const Napi::CallbackInfo& info);

void init(Napi::Env env, Napi::Object& exports);

}
}
}