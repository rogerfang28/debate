#pragma once 
#include "debate.pb.h"
#include "user.pb.h"
#include "../../../utils/DebateWrapper.h"
#include "../../../database/debate/DatabaseWrapper.h"

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