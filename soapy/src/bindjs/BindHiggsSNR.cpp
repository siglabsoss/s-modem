#include "BindCommonInclude.hpp"
#include "BindHiggsInclude.hpp"
#include "HiggsSNR.hpp"

using namespace std;

namespace siglabs {
namespace bind {
namespace higgsSNR {






napi_value GetHistory__wrapped(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() != 1) {
        Napi::TypeError::New(env, "Invalid argument count").ThrowAsJavaScriptException();
        return NULL;
    }


    const size_t arg0 = info[0].As<Napi::Number>().Int32Value();


    const std::vector<double> our_copy = soapy->higgsSNR->GetHistory(arg0);

    napi_value ret;
    auto status = napi_create_array_with_length(env, our_copy.size(), &ret);
    (void)status;

    unsigned i = 0;
    for(auto w : our_copy) {
        status = napi_set_element(env, ret, i, Napi::Number::New(env,w));
        i++;
    }
    return ret;
}









_NAPI_MEMBER(soapy->higgsSNR->,print_all,bool);
_NAPI_MEMBER(soapy->higgsSNR->,print_avg,bool);
_NAPI_MEMBER(soapy->higgsSNR->,expect_single_qpsk,bool);
_NAPI_MEMBER(soapy->higgsSNR->,avg_snr,double);
_NAPI_MEMBER(soapy->higgsSNR->,max_history,unsigned);

void init(Napi::Env env, Napi::Object& exports, const std::string role) {
    (void) role;

    napi_status status;
    (void)status;

    Object subtree = Object::New(env);

    _NAPI_MEMBER_INIT(subtree, print_all);
    _NAPI_MEMBER_INIT(subtree, print_avg);
    _NAPI_MEMBER_INIT(subtree, expect_single_qpsk);
    _NAPI_MEMBER_INIT(subtree, avg_snr);
    _NAPI_MEMBER_INIT(subtree, max_history);

    // custom
    _NAPI_GENERIC_INIT(subtree, GetHistory);


    exports.Set("snr", subtree);
}

}
}
}