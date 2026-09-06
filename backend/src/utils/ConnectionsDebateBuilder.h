#pragma once

#include "../debateModerator/DebateModerator.h"

#include <string>
#include <vector>

// Turns the two-phase generator's output into a real debate.
//
// Phase 1 gives a numbered list of claims. Phase 2 gives connections between
// those numbers: each one says which claims bear on which other claim, as a
// support or a challenge, with a basis. A connection is a Relationship in all
// but name -- several sources, one target, a role, a basis -- so the import is
// a topological walk that writes each connection once its target exists.
//
// Everything degrades by the line. A connection that will not parse, names a
// claim that does not exist, targets itself, or would close a cycle is DROPPED
// and reported; the rest are built. A claim no connection places is an ORPHAN
// and reported, not silently lost and not guessed into position. The result
// says exactly how much of what the model wrote was usable.
//
// Everything is created through DebateModerator::handleRequest, so the move
// log is written and statuses follow.
//
// AUTHORSHIP. Every claim is created by a synthetic account, AuthorName(),
// not by the person who asked for the debate; that person is joined as a
// member afterwards. Statuses here are per person and two rules drive them:
// a claim's creator sees it True, and a challenge's creator sees its target
// False. Put both sides of an extracted argument under the requester and
// every challenge refutes its own author's claim -- the root read False on
// the first real import, and the status panel was reporting, correctly, that
// one person was contradicting themself. Under a neutral author the requester
// sees everything Undetermined, which is what an extraction is: a map of the
// argument, not anybody's position. They take a side by acting.
class ConnectionsDebateBuilder {
public:
    // The account that authors every generated claim. One shared persona
    // across all generated debates; it holds no positions anyone reads.
    static std::string AuthorName();

    struct Connection {
        std::vector<int> from;  // 1-based claim numbers, as in the list
        int to = 0;
        bool opposes = false;
        std::string basis;
        std::string why;
        std::string joint_reason;
    };

    struct Result {
        bool ok = false;
        int debateId = 0;
        int claimsCreated = 0;
        int argumentsCreated = 0;
        int maxDepthReached = 0;
        // Connection lines that could not be used, with the reason for each.
        std::vector<std::string> dropped;
        // Claims (1-based) that no usable connection placed AND nothing else
        // depended on. Left out and reported, never guessed into position.
        std::vector<int> orphans;
        // Claims the model never placed but that OTHER connections target --
        // a hub it forgot. Attached under the root with no basis so the subtree
        // hanging off them survives, and counted here so the user is told the
        // placement is the builder's, not the model's. In one measured run a
        // forgotten claim 2 took five of fourteen claims down with it.
        int attachedToRoot = 0;
        // Of those, the ones attached because the AI's connections went round
        // in a circle (A under B, B under C, C under A). The circle is broken
        // at its most-targeted claim, which goes under the root; the one edge
        // that would close the circle again is dropped and reported. In one
        // measured run a three-claim circle took seven of thirty claims with
        // it before this.
        int cyclesBroken = 0;
        int groupsSplit = 0;
        int unknownBases = 0;
        std::string error;
    };

    // Phase-1 text -> claims, in order. Tolerant of any list marker ("1.",
    // "1)", "-", "*", "[1]") and of a code fence; blank lines are skipped.
    // Line 1 is the root by convention.
    static std::vector<std::string> ParseClaims(const std::string& text);

    // The same list with line 1 replaced by `root`, re-numbered from 1. Empty
    // `root` returns the text untouched. The topic the user typed is what the
    // debate is about; when it is already an assertion it IS the root, and the
    // model's own line 1 -- which drifts to the passage's headline fact in
    // about one run in three -- is only a paraphrase of it at best.
    static std::string WithRoot(const std::string& claimsText, const std::string& root);

    // Phase-2 text -> connections. One JSON object per line, or a JSON array
    // of them; each is parsed on its own so a bad one costs one link. Lines
    // that will not parse or lack required fields go to `dropped`.
    static std::vector<Connection> ParseConnections(const std::string& text,
                                                    std::vector<std::string>& dropped);

    static Result Build(const std::string& claimsText, const std::string& connectionsText,
                        int user_id, DebateModerator& moderator);
};
