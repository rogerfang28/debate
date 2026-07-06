#pragma once

#include "layout.pb.h"

class ErrorPageGenerator {
public:
    static ui::Page GenerateErrorPage();
    static ui::Component GenerateErrorPageMainLayout();
};