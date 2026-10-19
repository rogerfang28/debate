#pragma once

#include <map>
#include <string>
#include "moderator_to_vr.pb.h"
#include "debate_event.pb.h"

// database includes
#include "../database/sqlite/Database.h"
#include "../database/debate/DatabaseWrapper.h"

#include "../utils/DebateWrapper.h"
class DebateModerator {
public:
    // usersDb is the shared users.sqlite3 connection (owned by VirtualRenderer)
    // so the moderator and the renderer read/write the same user records.
    explicit DebateModerator(Database& usersDb);
    ~DebateModerator();

    moderator_to_vr::ModeratorToVRMessage handleRequest(debate_event::DebateEvent& event);
    int getUserId(const std::string& username) {
         return dbWrapper.users.getUserId(username);
     }
     DebateWrapper& getDebateWrapper() { return debateWrapper; }
     int createUserIfNotExist(const std::string& username);
     // A voice named on an event, resolved to a user id: blank means "the
     // submitter" (0); a name that is nobody yet becomes a user. That user
     // never logs in; it exists so a claim can be held by "the reporter".
     int resolveVoice(const std::string& name);
     // An anonymous visitor (visitorId) has just signed in as userId: moves
     // the visitor's place onto userId and takes the press they were signing
     // in to make off the visitor's record. True with `pending` filled (as
     // userId) when there is a press to replay through handleRequest.
     bool takePendingSignIn(const int& visitorId, const int& userId,
                            debate_event::DebateEvent* pending);

private:
    void handleDebateEvent(const int& user_id, debate_event::DebateEvent& event);
    moderator_to_vr::ModeratorToVRMessage buildResponseMessage(const int& user_id);
    int validateAuth(debate_event::DebateEvent& event);
    // The home shelf's cards, by debate id (see HomePageResponseGenerator).
    // Cleared by any event that may change a debate.
    std::map<int, moderator_to_vr::DebateTopic> homeCardCache;
    // database classes
    Database globalDb;
    DatabaseWrapper dbWrapper;
    DebateWrapper debateWrapper;
};
