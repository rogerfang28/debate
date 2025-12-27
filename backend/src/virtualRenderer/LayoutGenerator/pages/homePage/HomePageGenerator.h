#ifndef HOME_PAGE_GENERATOR_H
#define HOME_PAGE_GENERATOR_H

#include "../../../../../src/gen/cpp/layout.pb.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"

class HomePageGenerator {
public:
    static ui::Page GenerateHomePage(user_engagement::NoneInfo info);
};

#endif // HOME_PAGE_GENERATOR_H