#pragma once

#include "../../../../../src/gen/cpp/debate_event.pb.h"
#include "../../../utils/DebateWrapper.h"

class ConnectClaimsHandler {
public:
    static void ConnectClaims(
        const std::string& user,
        // const std::string& fromClaimId,
        // const std::string& toClaimId,
        const std::string& connection,
        DebateWrapper& debateWrapper
    );
    static void ConnectFromClaim(
        const std::string& user,
        const std::string& fromClaimId,
        DebateWrapper& debateWrapper
    );
    static void ConnectToClaim(
        const std::string& user,
        const std::string& toClaimId,
        DebateWrapper& debateWrapper
    );
    static void CancelConnectClaims(
        const std::string& user,
        DebateWrapper& debateWrapper
    );
    static void DeleteLinkById(
        const std::string& user,
        int linkId,
        DebateWrapper& debateWrapper
    );
};