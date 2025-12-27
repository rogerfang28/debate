#pragma once
#include "../../../../../src/gen/cpp/layout.pb.h"
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"

class DebatePageGenerator {
public:
    static ui::Page GenerateDebatePage(user_engagement::DebatingInfo debatingInfo);
};