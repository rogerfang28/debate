#pragma once

#include "../../../../../src/gen/cpp/debate_event.pb.h"
#include "../../../utils/DebateWrapper.h"

class ConnectClaimsHandler {
public:
    static void ConnectClaims(
        const int& user_id,
        // const std::string& fromClaimId,
        // const std::string& toClaimId,
        const std::string& connection,
        DebateWrapper& debateWrapper
    );
    static void ConnectFromClaim(
        const int& user_id,
        int fromClaimId,
        DebateWrapper& debateWrapper
    );
    static void ConnectToClaim(
        const int& user_id,
        int toClaimId,
        DebateWrapper& debateWrapper
    );
    static void CancelConnectClaims(
        const int& user_id,
        DebateWrapper& debateWrapper
    );
    static void DeleteLinkById(
        const int& user_id,
        int linkId,
        DebateWrapper& debateWrapper
    );
};