#pragma once

#include "../../../../../src/gen/cpp/layout.pb.h"
#include "../../../../../src/gen/cpp/user.pb.h"

class HistoryPageGenerator {
public:
    static ui::Page GenerateHistoryPage(user::User user);
    static ui::Component GenerateHistoryPageMainLayout();
    static ui::Component FillHistoryItems(user::User user, ui::Component mainLayout);
};