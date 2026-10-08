#ifndef HOME_PAGE_GENERATOR_H
#define HOME_PAGE_GENERATOR_H

#include "layout.pb.h"
#include "rendering_info.pb.h"

#include <string>

class HomePageGenerator {
public:
    static ui::Page GenerateHomePage(const rendering_info::HomePageRenderingInfo& info);
    // The home tab to draw for this view: "home", "explore" or "mine" (the
    // last for a signed-in reader only). The header lights the same one.
    static std::string ActiveTab(const user_engagement::HomeView& view, bool visitor);
};

#endif // HOME_PAGE_GENERATOR_H