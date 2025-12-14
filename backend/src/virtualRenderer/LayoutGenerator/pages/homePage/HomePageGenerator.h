#ifndef HOME_PAGE_GENERATOR_H
#define HOME_PAGE_GENERATOR_H

#include "../../../../../src/gen/cpp/layout.pb.h"
#include "../../../../../src/gen/cpp/debate_list.pb.h"

class HomePageGenerator {
public:
    static ui::Page GenerateHomePage(debate::DebateList usersDebates);
};

#endif // HOME_PAGE_GENERATOR_H