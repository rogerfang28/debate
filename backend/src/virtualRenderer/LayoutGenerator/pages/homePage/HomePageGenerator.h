#ifndef HOME_PAGE_GENERATOR_H
#define HOME_PAGE_GENERATOR_H

#include "../../../../../src/gen/cpp/layout.pb.h"
#include "../../../../../src/gen/cpp/user.pb.h"

class HomePageGenerator {
public:
    static ui::Page GenerateHomePage(user::User user);
    static ui::Component GenerateHomePageMainLayout();
    static ui::Component FillDebateTopics(user::User user, ui::Component mainLayout);
};

#endif // HOME_PAGE_GENERATOR_H