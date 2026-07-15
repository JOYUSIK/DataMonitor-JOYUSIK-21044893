#include "model/Sample.h"

Sample Sample::fromJson(const nlohmann::json& j) {
    return {
        j.at("id").get<std::string>(),
        j.at("name").get<std::string>(),
        j.at("avg_production_time").get<double>(),
        j.at("yield_rate").get<double>(),
        j.at("stock").get<int>()
    };
}
