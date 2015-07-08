#include "common/namespaces.h"
#include "common/typedefs.h"

#include "utility/json_input.h"

#include <iostream>

using namespace tcc;
int main(int argc, char *argv[]) {
    rapidjson::Document dom;
    parse_input(argc, argv, dom);
    if (dom.HasMember("hello")) {
        std::string world = dom["hello"].GetString();
        std::cout << "world = " << world << std::endl;
    } else {
        std::cout << "You didn't provide a key with the value 'hello'";
    }

    return 0;
}
