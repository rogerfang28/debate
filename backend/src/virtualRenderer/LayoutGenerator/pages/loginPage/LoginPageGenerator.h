#ifndef LOGIN_PAGE_GENERATOR_H
#define LOGIN_PAGE_GENERATOR_H

#include "../../../../../src/gen/cpp/layout.pb.h"

class LoginPageGenerator {
public:
    // errorMessage: shown in red above the form when non-empty (e.g. wrong password).
    // prefillUsername: pre-populates the username field so a failed attempt
    // doesn't force the user to retype it.
    static ui::Page GenerateLoginPage(const std::string& errorMessage = "",
                                      const std::string& prefillUsername = "");
    static ui::Component GenerateLoginPageMainLayout(const std::string& errorMessage = "",
                                                     const std::string& prefillUsername = "");
};

#endif // LOGIN_PAGE_GENERATOR_H