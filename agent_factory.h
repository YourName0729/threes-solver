#pragma once

#include "agent.h"
#include <memory>

class agent_factory {
public:

    static std::shared_ptr<agent> produce(const std::string& name, const std::string& args) {
        if      (name == "four_tuple_slider")     return std::make_shared<four_tuple_slider>(args);
        else if (name == "six_tuple_slider")      return std::make_shared<six_tuple_slider>(args);
        else if (name == "best_six_tuple_slider") return std::make_shared<best_six_tuple_slider>(args);
        return nullptr;
    }
};