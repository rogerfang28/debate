#pragma once

#include "../../../../src/gen/cpp/moderator_to_vr.pb.h"
#include <string>

class BuildPayload {
public:
    BuildPayload() = default;
    ~BuildPayload() = default;

    moderator_to_vr::ModeratorToVRMessage build(const std::string& username);
};