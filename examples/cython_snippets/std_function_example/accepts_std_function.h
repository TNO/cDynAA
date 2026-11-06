#include <functional>
#include <string>

double call_some_std_func(std::function<void(int,const std::string&)> callback) {
    callback(5,std::string("hello"));
    return 5.;
}
