#pragma once 
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../utils/DebateWrapper.h"
#include "../../../database/handlers/DatabaseWrapper.h"

class ModifyClaimHandler {
public:
    static void StartModifyClaim(
        DebateWrapper& debateWrapper,
        int user_id
    );

    static void CancelModifyClaim(
        DebateWrapper& debateWrapper,
        int user_id
    );

    static void SubmitModifyClaim(
        DebateWrapper& debateWrapper,
        int user_id
    );
};