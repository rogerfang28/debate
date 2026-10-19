#pragma once

#include <string>

class DebateModerator;
class DebateWrapper;

// Drives a guided walkthrough: a scenario replayed one step per click, with
// the explanation for each step read from the script file.
//
// The only state kept between clicks is the reader's position -- which script,
// which step -- on their user record. Nothing is held in memory, so a
// walkthrough survives a server restart: each click builds a fresh importer and
// re-attaches it to the debate built so far. The alternative, keeping a live
// importer per reader, would have made a restart mid-walkthrough into a
// dead-end that only a support conversation could explain.
namespace TutorialHandler {

// Begins `script` (empty for the default) and immediately performs its first
// step, so the reader lands on something already happening rather than on an
// empty debate with instructions.
void Start(const std::string& script, const int& user_id, DebateModerator& moderator);

// Performs the next step. Does nothing once the script is finished.
void Next(const int& user_id, DebateModerator& moderator);

// Leaves the walkthrough. Deliberately does NOT delete the debate: the point
// of watching one get built is being able to take it apart afterwards.
void Exit(const int& user_id, DebateWrapper& debateWrapper);

} // namespace TutorialHandler
