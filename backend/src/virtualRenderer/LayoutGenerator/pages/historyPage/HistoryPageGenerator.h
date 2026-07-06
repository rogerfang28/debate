#pragma once

#include "layout.pb.h"
#include "user.pb.h"

class HistoryPageGenerator {
public:
    static ui::Page GenerateHistoryPage(user::User user);
    static ui::Component GenerateHistoryPageMainLayout();
    static ui::Component FillHistoryItems(user::User user, ui::Component mainLayout);
};