#include "ConnectClaimsHandler.h"
#include "../../../utils/Log.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"

void ConnectClaimsHandler::ConnectClaims(
    const std::string& user,
    const std::string& fromClaimId,
    const std::string& toClaimId,
    const std::string& connection,
    DebateWrapper& debateWrapper
) {
    // this one should actually update the protobuf
}

void ConnectClaimsHandler::ConnectFromClaim(
    const std::string& user,
    const std::string& fromClaimId,
    DebateWrapper& debateWrapper
) {
    // take user proto and put connect from claim);
    user_engagement::UserEngagement userEngagement;
    userEngagement = debateWrapper.getUserProtobufByUsername(user).engagement();
    
}

void ConnectClaimsHandler::ConnectToClaim(
    const std::string& user,
    const std::string& toClaimId,
    DebateWrapper& debateWrapper
) {
    // take user proto and put connect to claim
}

void ConnectClaimsHandler::CancelConnectClaims(
    const std::string& user,
    DebateWrapper& debateWrapper
) {
    // take user proto and cancel connect claims

}