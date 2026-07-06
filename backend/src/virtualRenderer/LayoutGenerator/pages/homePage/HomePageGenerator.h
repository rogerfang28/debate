#ifndef HOME_PAGE_GENERATOR_H
#define HOME_PAGE_GENERATOR_H

#include "layout.pb.h"
#include "rendering_info.pb.h"

class HomePageGenerator {
public:
    static ui::Page GenerateHomePage(const rendering_info::HomePageRenderingInfo& info);
    static ui::Component GenerateHomePageMainLayout(const rendering_info::HomePageRenderingInfo& info);
    static ui::Component FillDebateTopics(const rendering_info::HomePageRenderingInfo& info, ui::Component mainLayout);
};

#endif // HOME_PAGE_GENERATOR_H