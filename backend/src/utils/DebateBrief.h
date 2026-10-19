#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "collection.pb.h"

// The Brief: a debate reduced to what someone catching up needs.
//
// Two halves, kept apart on purpose.
//
// The STRUCTURE -- who holds the main claim true or false, which challenges
// exist, which are still unanswered and by whom, which one the whole case
// hangs on, what nobody disputes -- is computed here from the collection.
// It costs nothing, never lies, and is rebuilt on every page.
//
// The WORDS -- a one-sentence verdict, a three-sentence story, a short name
// for each battle site -- come from the model, which is given the structure
// and asked to write only those. It cannot add a site or get whose-turn
// wrong, because it is never asked for them.
namespace DebateBrief {

// One line of an exchange: a claim, and whose it is.
struct Move {
    int claimId = 0;
    std::string who;
    std::string text;
    // The claim is offered AGAINST something in this site.
    bool challenge = false;
    // The challenge answers an earlier challenge, rather than attacking the
    // site's own claim.
    bool reply = false;
    // The challenge is aimed at the reasoning for a claim, not the claim.
    bool againstReasoning = false;
};

// One battle site: a claim somebody challenged, and everything that
// followed. The first move is the claim; then each challenge to it, each
// followed by its replies, in the order they were made. Two people who
// challenge the same claim are at the same site.
struct Site {
    // The challenged claim's id -- stable, and what the words are filed
    // under.
    int id = 0;
    int targetClaimId = 0;
    std::vector<Move> moves;
    // Someone still owes a reply.
    bool open = false;
    // Who, when open: everyone at this site who still owes a reply.
    // Comma-joined for display.
    std::string turn;
    // When not open: how it closed ("Answered", "Paul conceded").
    std::string outcome;
    // How far the challenged claim is from the main claim. 0 = the main
    // claim itself.
    int depth = 0;
    // The open site nearest the main claim: what the conclusion hangs on.
    bool hinge = false;
};

struct Stance {
    std::string who;
    debate::ClaimStatus status = debate::UNDETERMINED;
};

struct Facts {
    int rootClaimId = 0;
    std::string rootText;
    std::string rootHolder;
    // Everyone with a view on the main claim: true first, then false, then
    // undecided.
    std::vector<Stance> stances;
    // Hinge first, then the other open sites nearest the root first, then
    // the answered ones.
    std::vector<Site> sites;
    // Claims nobody challenged that more than one person holds true.
    std::vector<Move> commonGround;
    // Every claim in the order it was made.
    std::vector<Move> timeline;
    std::vector<std::string> sources;
    // Holder -> the challenge claims they have not answered. Same rule as
    // the Debate tab's open-challenges panel.
    std::map<std::string, std::vector<int>> openByHolder;
};

// Display name for a user id (the voice of a claim, or its entrant).
using NameOf = std::function<std::string(int)>;

Facts Compute(const debate::Collection& collection, const NameOf& nameOf);

// The structure as text: what the model is shown, and what the fingerprint
// is taken over. Deterministic for a given debate.
std::string FactsText(const Facts& facts);
std::string Fingerprint(const Facts& facts);

// The full prompt for the model: rules, the structure, the JSON it must
// return.
std::string Prompt(const Facts& facts, const std::string& topic);

// What the model wrote.
struct Prose {
    std::string verdict;
    std::string story;
    // Site id -> its name.
    std::map<int, std::string> siteNames;
};

// Reads the model's reply. A ``` fence around the JSON is tolerated.
// Returns false with a reason when the reply is not usable.
bool ParseProse(const std::string& reply, Prose* prose, std::string* error);

// Storage form, for BRIEFS and the user record.
std::string SerializeProse(const Prose& prose);
bool DeserializeProse(const std::string& json, Prose* prose);

}  // namespace DebateBrief
