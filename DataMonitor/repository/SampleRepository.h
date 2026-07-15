#pragma once
#include <vector>
#include <string>
#include "model/Sample.h"

class SampleRepository {
public:
    std::vector<Sample> findAll(const std::string& dataPath) const;
};
