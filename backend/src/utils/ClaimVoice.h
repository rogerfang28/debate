#pragma once
#include "debate.pb.h"

// The user WHOSE claim this is: the voice if one was named, else the entrant.
//
// A reader who enters "the reporter says X" does not hold X; the reporter
// does. So truth is seeded for this user, "is this my claim" fallbacks in
// the renderers compare against it, and the text formatter prints it as
// the author. Edit and delete rights stay with creator_id -- the entrant
// typed it and can fix it; the voice never logs in.
inline int ClaimVoiceId(const debate::Claim& claim) {
    return claim.voice_id() > 0 ? claim.voice_id() : claim.creator_id();
}
