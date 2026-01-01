#ifndef LOGIN_PAGE_GENERATOR_H
#define LOGIN_PAGE_GENERATOR_H

#include "../../../../../src/gen/cpp/layout.pb.h"

class LoginPageGenerator {
public:
    static ui::Page GenerateLoginPage();
    static ui::Component GenerateLoginPageMainLayout();
};

#endif // LOGIN_PAGE_GENERATOR_H