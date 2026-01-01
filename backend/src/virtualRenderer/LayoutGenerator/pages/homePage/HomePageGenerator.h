#ifndef HOME_PAGE_GENERATOR_H
#define HOME_PAGE_GENERATOR_H

#include "../../../../../src/gen/cpp/layout.pb.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"

class HomePageGenerator {
public:
    static ui::Page GenerateHomePage(user_engagement::UserEngagement userEngagement);
    static ui::Component GenerateHomePageMainLayout();
    static ui::Component FillDebateTopics(user_engagement::UserEngagement userEngagement, ui::Component mainLayout);
};

#endif // HOME_PAGE_GENERATOR_H