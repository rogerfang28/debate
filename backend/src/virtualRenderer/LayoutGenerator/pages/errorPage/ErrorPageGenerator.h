#pragma once

#include "../../../../../src/gen/cpp/layout.pb.h"

class ErrorPageGenerator {
public:
    static ui::Page GenerateErrorPage();
    static ui::Component GenerateErrorPageMainLayout();
};