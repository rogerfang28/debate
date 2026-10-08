#ifndef LOGIN_PAGE_GENERATOR_H
#define LOGIN_PAGE_GENERATOR_H

#include "layout.pb.h"

class LoginPageGenerator {
public:
    // errorMessage: shown in red above the form when non-empty (e.g. wrong password).
    // prefillUsername: pre-populates the username field so a failed attempt
    // doesn't force the user to retype it.
    static ui::Page GenerateLoginPage(const std::string& errorMessage = "",
                                      const std::string& prefillUsername = "");
    static ui::Component GenerateLoginPageMainLayout(const std::string& errorMessage = "",
                                                     const std::string& prefillUsername = "");
    // The username / password / guest / Google form, shared by the login page
    // and the sign-in modal. inModal gives the submit button the modal's own
    // id (signInSubmitButton) and makes "continue as guest" a small link.
    static void AddLoginForm(ui::Component* parent, const std::string& prefillUsername, bool inModal);
};

#endif // LOGIN_PAGE_GENERATOR_H