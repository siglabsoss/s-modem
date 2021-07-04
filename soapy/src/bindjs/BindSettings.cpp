
#include "BindCommonInclude.hpp"
#include "BindHiggsInclude.hpp"

#include "TxSchedule.hpp"
#include "ScheduleData.hpp"
#include "FlushHiggs.hpp"

using namespace std;

#include "BindFlushHiggs.hpp"
namespace siglabs {
namespace bind {
namespace settings {




std::vector<std::string> vecForPath(const std::string pathStr) {
    std::vector<std::string> path;

    std::string token;
    std::istringstream stream(pathStr);

    while(std::getline(stream, token, '.')) {
        path.push_back(token);
    }

    return path;
}

double getDoubleDefault(double _default, std::string pathStr) {
    return soapy->settings->vectorGetDefault<double>(_default, vecForPath(pathStr));
}
_NAPI_BODY(getDoubleDefault,double,double,string);

double getIntDefault(int _default, std::string pathStr) {
    return soapy->settings->vectorGetDefault<int>(_default, vecForPath(pathStr));
}
_NAPI_BODY(getIntDefault,int,int,string);

bool getBoolDefault(bool _default, std::string pathStr) {
    return soapy->settings->vectorGetDefault<bool>(_default, vecForPath(pathStr));
}
_NAPI_BODY(getBoolDefault,bool,bool,string);

std::string getStringDefault(std::string _default, std::string pathStr) {
    return soapy->settings->vectorGetDefault<std::string>(_default, vecForPath(pathStr));
}
_NAPI_BODY(getStringDefault,string,string,string);



void init(Napi::Env env, Napi::Object& exports) {

  napi_status status;
  (void)status;

  exports.Set("getDoubleDefault", Napi::Function::New(env, getDoubleDefault__wrapped));
  exports.Set("getBoolDefault", Napi::Function::New(env, getBoolDefault__wrapped));
  exports.Set("getIntDefault", Napi::Function::New(env, getIntDefault__wrapped));
  exports.Set("getStringDefault", Napi::Function::New(env, getStringDefault__wrapped));

}


}
}
}