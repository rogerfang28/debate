#ifndef LOGIN_PAGE_GENERATOR_H
#define LOGIN_PAGE_GENERATOR_H

#include "layout.pb.h"

// There is no login page: a reader without a session is signed in as a
// visitor and lands on the home page, and signing in is the sign-in modal
// (LayoutGenerator's BuildSignInModal). This is the modal's form.
class LoginPageGenerator {
public:
    // The username / password / Google form, with "continue as guest" as a
    // small link under it. prefillUsername pre-populates the username field
    // so a failed attempt doesn't force the user to retype it.
    static void AddLoginForm(ui::Component* parent, const std::string& prefillUsername);
};

#endif // LOGIN_PAGE_GENERATOR_H
