#include "ImportHandler.h"

#include "../../DebateModerator.h"
#include "../MoveUserHandler/MoveUserHandler.h"
#include "../../../utils/DebateImport.h"
#include "../../../utils/DebateWrapper.h"
#include "../../../utils/Log.h"
#include "../../../utils/ScenarioImporter.h"
#include "user.pb.h"

#include <algorithm>
#include <set>
#include <string>
#include <vector>

namespace {

using user_engagement::HomeView;
using user_engagement::ImportPreview;

void clearImport(HomeView* view) {
    view->clear_import_open();
    view->clear_import_text();
    view->clear_import_preview();
}

// Home, on the admin's own profile, with the import view in the state
// `change` leaves it.
template <typename Change>
void onImportView(int user_id, DebateModerator& moderator, Change change) {
    DebateWrapper& dw = moderator.getDebateWrapper();
    MoveUserHandler::GoHome(user_id, dw);
    user::User userProto = dw.getUserProtobuf(user_id);
    HomeView* view = userProto.mutable_engagement()->mutable_home_view();
    view->set_profile_user_id(user_id);
    view->clear_profile_panel();
    view->clear_profile_error();
    view->clear_profile_draft();
    view->set_create_open(false);
    change(view);
    dw.updateUserProtobuf(user_id, userProto);
}

ImportPreview previewOf(const debate_import::Plan& plan, const std::string& admin_name,
                        DebateModerator& moderator) {
    ImportPreview p;
    p.set_done(true);
    p.set_format(plan.format);
    p.set_root(plan.root);
    p.set_claims(plan.claims);
    p.set_supports(plan.supports);
    p.set_challenges(plan.challenges);
    for (const std::string& e : plan.errors) p.add_errors(e);
    for (const std::string& w : plan.warnings) p.add_warnings(w);
    const int adminId = moderator.getUserId(admin_name);
    for (const std::string& voice : plan.voices) {
        p.add_voices(voice);
        const int id = moderator.getUserId(voice);
        if (id == -1) {
            p.add_new_voices(voice);
        } else if (id != adminId) {
            // Claims in this voice are held by that account. A quoted source
            // never signs in; someone who does should know they are being
            // quoted, so say so before anything is written in their name.
            const user::User u = moderator.getDebateWrapper().getUserProtobuf(id);
            if (!u.password_hash().empty() || !u.google_sub().empty()) {
                p.add_warnings("\"" + voice + "\" is an account that signs in; the claims in that voice "
                               "will be held by that person.");
            }
        }
    }
    return p;
}

int claimCount(DebateModerator& moderator, int debate_id) {
    return static_cast<int>(moderator.getDebateWrapper().getStatementsForDebate(debate_id).size());
}

}  // namespace

namespace ImportHandler {

ImportPreview Check(const std::string& text, const std::string& admin_name, DebateModerator& moderator) {
    return previewOf(debate_import::Read(text), admin_name, moderator);
}

void Open(int user_id, DebateModerator& moderator) {
    onImportView(user_id, moderator, [](HomeView* view) {
        clearImport(view);
        view->set_import_open(true);
    });
}

void Close(int user_id, DebateModerator& moderator) {
    onImportView(user_id, moderator, [](HomeView* view) { clearImport(view); });
}

void Preview(const std::string& text, int user_id, DebateModerator& moderator) {
    const std::string admin = moderator.getDebateWrapper().getUserProtobuf(user_id).username();
    const ImportPreview preview = Check(text, admin, moderator);
    Log::info("[Import] " + admin + " previewed " + std::to_string(text.size()) + " bytes: " +
              (preview.format().empty() ? "unreadable" : preview.format()) + ", " +
              std::to_string(preview.claims()) + " claims, " + std::to_string(preview.errors_size()) +
              " error(s)");
    onImportView(user_id, moderator, [&](HomeView* view) {
        clearImport(view);
        view->set_import_open(true);
        // Too much text is refused by the preview and not kept: the user
        // record is read and written on every request.
        if (text.size() <= debate_import::kMaxBytes) view->set_import_text(text);
        *view->mutable_import_preview() = preview;
    });
}

void Publish(const std::string& text, int user_id, DebateModerator& moderator) {
    DebateWrapper& dw = moderator.getDebateWrapper();
    const user::User before = dw.getUserProtobuf(user_id);
    const HomeView& stored = before.engagement().home_view();
    const std::string admin = before.username();

    if (!stored.import_open()) {
        Log::info("[Import] Publish from " + admin + " ignored: the import view is not open.");
        return;
    }
    // Only what was previewed, unchanged, is published.
    if (!stored.import_preview().done() || stored.import_text() != text ||
        stored.import_preview().errors_size() > 0) {
        const bool changed = stored.import_preview().done() && stored.import_text() != text;
        Preview(text, user_id, moderator);
        if (changed) {
            onImportView(user_id, moderator, [](HomeView* view) {
                view->mutable_import_preview()->set_publish_error(
                    "The text changed after the preview, so nothing was published. It has been checked "
                    "again; press Publish once more if the preview looks right.");
                view->set_import_open(true);
            });
        }
        return;
    }

    const debate_import::Plan plan = debate_import::Read(text);
    if (!plan.ok()) {
        Preview(text, user_id, moderator);
        return;
    }

    // Every step reads and rewrites this record; it should not carry the text.
    onImportView(user_id, moderator, [](HomeView* view) { clearImport(view); });

    const std::vector<int> oldIds = dw.getUserDebateIds(user_id);
    const std::set<int> earlier(oldIds.begin(), oldIds.end());

    scenario::Importer importer(moderator);
    importer.AliasUser(plan.actor, admin);

    const int total = plan.scenario.steps_size();
    int failed = -1;
    bool grafted = false;
    for (int i = 0; i < total; ++i) {
        if (!plan.scenario.steps(i).has_action()) continue;
        if (!importer.RunAction(plan.scenario.steps(i).action())) {
            failed = i;
            break;
        }
        // The importer takes the admin's newest debate as the one it made. If
        // the create was refused, that is an older debate: stop before a
        // single claim lands on it.
        if (plan.scenario.steps(i).action().event_type() == "CREATE_DEBATE") {
            if (importer.debate_id() <= 0 || earlier.count(importer.debate_id()) > 0) {
                failed = i;
                grafted = true;
                break;
            }
        }
    }

    const int debateId = grafted ? 0 : importer.debate_id();
    const int made = debateId > 0 ? claimCount(moderator, debateId) : 0;

    if (failed < 0 && debateId > 0 && made == plan.claims) {
        Log::info("[Import] " + admin + " imported debate " + std::to_string(debateId) + ": " +
                  std::to_string(made) + " claims (" + std::to_string(plan.supports) + " supports, " +
                  std::to_string(plan.challenges) + " challenges), " + plan.format + ", \"" + plan.root + "\"");
        moderator.activity().record("import", user_id, admin, debateId, 0,
                                    std::to_string(made) + (made == 1 ? " claim" : " claims"));
        MoveUserHandler::EnterDebate(debateId, user_id, dw);
        user::User after = dw.getUserProtobuf(user_id);
        HomeView* view = after.mutable_engagement()->mutable_home_view();
        clearImport(view);
        view->clear_profile_user_id();
        dw.updateUserProtobuf(user_id, after);
        return;
    }

    std::string message;
    if (failed >= 0) {
        const debate_test::TestAction& a = plan.scenario.steps(failed).action();
        message = "Publishing stopped at step " + std::to_string(failed + 1) + " of " + std::to_string(total) +
                  " (" + a.event_type();
        if (failed < static_cast<int>(plan.stepLines.size()) && plan.stepLines[failed] > 0) {
            message += ", outline line " + std::to_string(plan.stepLines[failed]);
        }
        message += ")";
        message += grafted ? ": the debate could not be created." : ".";
    } else {
        message = "Every step ran, but the debate has " + std::to_string(made) + " claims where " +
                  std::to_string(plan.claims) + " were expected; the server refused some of them.";
    }
    if (debateId > 0) {
        message += " A partial debate may exist: debate " + std::to_string(debateId) +
                   " (\"" + plan.root + "\"), with " + std::to_string(made) +
                   " claim(s). Check it under Your debates, and delete it before trying again.";
    } else {
        message += " No debate was created.";
    }
    Log::warn("[Import] " + admin + ": " + message);

    ImportPreview preview = previewOf(plan, admin, moderator);
    preview.set_publish_error(message);
    onImportView(user_id, moderator, [&](HomeView* view) {
        clearImport(view);
        view->set_import_open(true);
        view->set_import_text(text);
        *view->mutable_import_preview() = preview;
    });
}

}  // namespace ImportHandler
