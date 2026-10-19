#include "TutorialHandler.h"

#include "../../DebateModerator.h"
#include "../../../utils/DebateWrapper.h"
#include "../../../utils/Log.h"
#include "../../../utils/ScenarioImporter.h"
#include "../../../utils/TutorialLibrary.h"
#include "user.pb.h"

namespace {

// The name a script uses for the person reading it. Every other name in a
// script is an ordinary account created on demand.
//
// This is what makes a walkthrough something you are IN rather than something
// you watch: the claims attributed to YOU are made by the reader's own account,
// so the statuses on screen are theirs, the challenges are against them, and
// the concession at the end is one they watched themselves make.
constexpr const char* kViewerName = "YOU";

user_engagement::TutorialStatus* statusOf(user::User& userProto) {
    return userProto.mutable_engagement()->mutable_tutorial();
}

// Runs step `index` of `script`. Returns false and fills `error` if the step
// would not replay.
//
// The importer is built fresh and re-attached every time rather than kept
// alive, so this works identically on the first click and on the first click
// after a restart.
bool runStep(const tutorial::Tutorial& script,
             int index,
             const std::string& viewerName,
             int& debate_id,
             DebateModerator& moderator,
             std::string& error) {
    if (index < 0 || index >= script.steps_size()) {
        error = "That tutorial step does not exist.";
        return false;
    }

    const tutorial::TutorialStep& step = script.steps(index);
    if (step.actions_size() == 0) {
        // A narration-only step. Nothing to replay, and that is not a failure --
        // the closing step is usually exactly this.
        return true;
    }

    // One importer for the whole step, so the actions in it share the id maps.
    // A fresh importer per action would renumber the claims each time and send
    // the second action to the wrong one.
    scenario::Importer importer(moderator);
    importer.AliasUser(kViewerName, viewerName);
    if (debate_id > 0) {
        importer.Attach(debate_id);
    }

    for (int i = 0; i < step.actions_size(); ++i) {
        const debate_test::TestAction& action = step.actions(i);
        if (!importer.RunAction(action)) {
            error = "Tutorial step " + std::to_string(index + 1) + ", action " +
                    std::to_string(i + 1) + " (" + action.event_type() +
                    ") could not be replayed.";
            return false;
        }
        // The debate is created by the first step, and every action after it in
        // that same step needs the importer pointed at the result -- otherwise
        // a JOIN_DEBATE in the same step joins whatever debate id the script
        // literally wrote.
        if (debate_id <= 0 && importer.debate_id() > 0) {
            debate_id = importer.debate_id();
        }
    }
    return true;
}

} // namespace

namespace TutorialHandler {

void Start(const std::string& script, const int& user_id, DebateModerator& moderator) {
    DebateWrapper& debateWrapper = moderator.getDebateWrapper();
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    user_engagement::TutorialStatus* status = statusOf(userProto);
    status->Clear();

    const std::string name = script.empty() ? tutorials::DefaultName() : script;
    bool ok = false;
    std::string error;
    const tutorial::Tutorial loaded = tutorials::Load(name, ok, error);
    if (!ok) {
        // Reported on the page rather than only logged: an empty or misnamed
        // tutorials directory is a setup mistake, and a button that appears to
        // do nothing is the worst way to report one.
        Log::warn("[Tutorial] Could not start '" + name + "': " + error);
        status->set_active(true);
        status->set_script(name);
        status->set_error(error);
        debateWrapper.updateUserProtobuf(user_id, userProto);
        return;
    }

    Log::info("[Tutorial] Starting '" + name + "' (" + std::to_string(loaded.steps_size()) +
              " steps) for user " + std::to_string(user_id));

    status->set_active(true);
    status->set_script(name);
    status->set_step_index(0);
    status->set_total_steps(loaded.steps_size());
    status->set_finished(loaded.steps_size() == 1);

    int debate_id = 0;
    std::string stepError;
    if (!runStep(loaded, 0, userProto.username(), debate_id, moderator, stepError)) {
        status->set_error(stepError);
    }
    status->set_debate_id(debate_id);

    // The first step created the debate, which moved this user into it. That
    // write happened inside handleRequest on a COPY of the user record read
    // before it, so this one has to be re-read or saving it would put the
    // reader back on the home page they just left.
    user::User afterStep = debateWrapper.getUserProtobuf(user_id);
    *afterStep.mutable_engagement()->mutable_tutorial() = *status;
    debateWrapper.updateUserProtobuf(user_id, afterStep);
}

void Next(const int& user_id, DebateModerator& moderator) {
    DebateWrapper& debateWrapper = moderator.getDebateWrapper();
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    user_engagement::TutorialStatus status = userProto.engagement().tutorial();

    if (!status.active() || status.finished()) {
        return;
    }

    bool ok = false;
    std::string error;
    const tutorial::Tutorial loaded = tutorials::Load(status.script(), ok, error);
    if (!ok) {
        statusOf(userProto)->set_error(error);
        debateWrapper.updateUserProtobuf(user_id, userProto);
        return;
    }

    // The script is re-read every click, so it can have been edited -- or
    // shortened -- since this walkthrough began. Clamping beats indexing past
    // the end of a file somebody trimmed while a reader was inside it.
    if (loaded.steps_size() != status.total_steps()) {
        status.set_total_steps(loaded.steps_size());
    }

    const int nextIndex = status.step_index() + 1;
    if (nextIndex >= loaded.steps_size()) {
        status.set_finished(true);
        status.set_step_index(loaded.steps_size() - 1);
        *statusOf(userProto) = status;
        debateWrapper.updateUserProtobuf(user_id, userProto);
        return;
    }

    int debate_id = status.debate_id();
    std::string stepError;
    if (!runStep(loaded, nextIndex, userProto.username(), debate_id, moderator, stepError)) {
        // The step index does NOT advance on failure. A walkthrough that
        // skipped past a step it could not perform would keep narrating a
        // debate that no longer matches what is on screen.
        Log::warn("[Tutorial] " + stepError);
        statusOf(userProto)->set_error(stepError);
        debateWrapper.updateUserProtobuf(user_id, userProto);
        return;
    }

    status.clear_error();
    status.set_step_index(nextIndex);
    status.set_debate_id(debate_id);
    status.set_finished(nextIndex + 1 >= loaded.steps_size());

    // Re-read for the same reason as in Start: the step just performed may have
    // written this user's engagement, and this copy predates it.
    user::User afterStep = debateWrapper.getUserProtobuf(user_id);
    *afterStep.mutable_engagement()->mutable_tutorial() = status;
    debateWrapper.updateUserProtobuf(user_id, afterStep);
}

void Exit(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    userProto.mutable_engagement()->clear_tutorial();
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

} // namespace TutorialHandler
