// test/tests/test_step_view_layout.cc
//
// Generates the step view layout protobuf for a debate and saves it as a .pbtxt
// file for inspection. This uses the same production code path as the virtual
// renderer: BuildCollection → FullDebatePageInfoParser → StepView.
//
// Usage: call downloadPbtxt(debate_id) at the end of a test to dump the step
// view for that debate. The output goes to test/build/step_view_<id>.pbtxt.

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include "utils/Log.h"

#include "debate.pb.h"
#include "debate_event.pb.h"
#include "user.pb.h"
#include "user_engagement.pb.h"
#include "moderator_to_vr.pb.h"
#include "layout.pb.h"
#include "collection.pb.h"
#include "rendering_info.pb.h"
#include "test_scenario.pb.h"
#include "database/sqlite/Database.h"
#include "database/debate/DatabaseWrapper.h"
#include "database/virtualrenderer/VRUserDatabase.h"
#include "utils/DebateWrapper.h"
#include "debateModerator/DebateModerator.h"
#include "debateModerator/buildResponse/debatePageResponse/BuildCollection.h"
#include "virtualRenderer/LayoutGenerator/LayoutGenerator.h"
#include "virtualRenderer/LayoutGenerator/ComponentGenerator.h"
#include "virtualRenderer/LayoutGenerator/pages/debatePage/StepView/StepView.h"
#include "virtualRenderer/LayoutGenerator/pages/debatePage/FullDebateView/FullDebatePageInfoParser.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/json/json.h"

// ---------------------------------------------------------------------------
// downloadJson — Generate step view layout for a debate and save as .json
// ---------------------------------------------------------------------------
// debate_id: the debate to generate the step view for
// viewer_username: which user's perspective to use (determines per-user claim statuses)
// output_path: where to save the .json file (relative to cwd, usually test/build/)
// description: human-readable label for this snapshot (e.g. "Step 5: B submits challenge")
//
// This function:
// 1. Creates a DebateModerator (reuses the existing DB via DB_PATH env var)
// 2. Finds all users in the debate
// 3. Builds the collection via BuildCollection::BuildForDebateAndUsers()
// 4. Parses the full debate view info via FullDebatePageInfoParser
// 5. Generates the step view page via StepView::GenerateStepViewPage()
// 6. Adds a description text component at the bottom of the page
// 7. Serializes the resulting ui::Page to a .json file
// ---------------------------------------------------------------------------
void downloadJson(int debate_id, const std::string& viewer_username = "A",
                  const std::string& output_path = "",
                  const std::string& description = "") {
    // Use the same DB_PATH that the test fixture set
    const char* db_path = getenv("DB_PATH");
    if (!db_path || std::string(db_path).empty()) {
        Log::error("[downloadJson] DB_PATH not set — was a test fixture SetUp() called?");
        return;
    }

    // DB handle wired to DB_PATH. Declared before the moderator so that it
    // outlives it -- the moderator holds a reference to this connection.
    Database wrapperDb(db_path);

    // Create moderator (uses existing DB)
    DebateModerator moderator(wrapperDb);

    // Used only to reach a DebateWrapper for read-only collection building in
    // tests (DebateModerator no longer exposes its internal DebateWrapper).
    // Debates and users share one file here; production splits them.
    DatabaseWrapper wrapperDbWrapper(wrapperDb, wrapperDb);
    DebateWrapper debateWrapper(wrapperDbWrapper);

    // Find all users in the debate
    std::vector<int> user_ids = debateWrapper.findUsersInDebate(debate_id);
    if (user_ids.empty()) {
        Log::error("[downloadJson] No users found in debate " + std::to_string(debate_id));
        return;
    }

    // Get the viewer's user ID
    int viewer_user_id = moderator.getUserId(viewer_username);
    if (viewer_user_id <= 0) {
        Log::error("[downloadJson] Viewer user '" + viewer_username + "' not found");
        return;
    }

    // Build the collection for all users in the debate
    debate::Collection collection = BuildCollection::BuildForDebateAndUsers(
        debate_id, user_ids, debateWrapper);

    Log::info("[downloadJson] Collection: " + std::to_string(collection.claims_by_id_size())
              + " claims, " + std::to_string(collection.links_by_id_size()) + " links");

    // Parse the full debate view info (tree structure, steps, etc.)
    rendering_info::FullDebateViewInfo fullDebateInfo =
        FullDebatePageInfoParser::ParseFullDebateViewInfo(
            collection, viewer_user_id, viewer_username);

    Log::info("[downloadJson] FullDebateViewInfo: " + std::to_string(fullDebateInfo.steps_size())
              + " steps, " + std::to_string(fullDebateInfo.full_debate_tree().nodes_size())
              + " tree nodes, " + std::to_string(fullDebateInfo.full_debate_tree().links_size())
              + " tree links");

    // Create VRUserDatabase for StepView
    Database db(db_path);
    VRUserDatabase userDb(db);

    // Generate the step view page
    user::User viewerUser = debateWrapper.getUserProtobuf(viewer_user_id);
    ui::Page page = StepView::GenerateStepViewPage(fullDebateInfo, collection, userDb, viewerUser);

    Log::info("[downloadJson] Generated step view page: page_id=" + page.page_id()
              + ", title=" + page.title());

    // Add description as the last component of the page (a visible label at the bottom)
    if (!description.empty()) {
        ui::Component descComponent = ComponentGenerator::createText(
            "stepDescription",
            description,
            "text-xs",
            "text-gray-500",
            "",
            "border-t border-gray-700 mt-4 pt-2"
        );
        page.add_components()->CopyFrom(descComponent);
    }

    // Serialize to JSON using protobuf's built-in JSON utility
    std::string json_output;
    google::protobuf::json::PrintOptions opts;
    opts.add_whitespace = true;  // pretty-print for readability
    auto status = google::protobuf::json::MessageToJsonString(page, &json_output, opts);
    if (!status.ok()) {
        Log::error("[downloadJson] Failed to serialize to JSON: " + status.ToString());
        return;
    }

    // Determine output path
    std::string out_file = output_path;
    if (out_file.empty()) {
        out_file = "step_view_" + std::to_string(debate_id) + ".json";
    }

    // Write to file
    std::ofstream out(out_file);
    if (out.is_open()) {
        out << json_output;
        out.close();
        Log::info("[downloadJson] Saved step view to: " + out_file
                  + (description.empty() ? "" : " — " + description));
    } else {
        Log::error("[downloadJson] Failed to open output file: " + out_file);
    }
}

// ---------------------------------------------------------------------------
// The challenge modal, rendered through the real path.
//
// A browser is the wrong instrument for "does the modal come up": it depends
// on a pane being visible and a server being alive. This drives the handlers a
// click would drive and renders the page a click would get, then reads what is
// in it. Two cases, deliberately the same shape: challenging a claim, and
// challenging an argument picked by its pill.
// ---------------------------------------------------------------------------
#include "debateModerator/event-handlers/DebateHandler/DebateHandler.h"
#include "debateModerator/event-handlers/MoveUserHandler/MoveUserHandler.h"
#include "debateModerator/event-handlers/AddClaimHandler/AddClaimHandler.h"
#include "debateModerator/event-handlers/ChallengeHandler/ChallengeHandler.h"
#include "debateModerator/buildResponse/debatePageResponse/DebatePageResponseGenerator.h"
#include <cstdio>

namespace {

user::User modalTestUser(const std::string& name) {
    user::User u;
    u.set_username(name);
    u.mutable_engagement()->set_current_action(user_engagement::ACTION_HOME);
    return u;
}

std::vector<uint8_t> modalTestBytes(const user::User& u) {
    std::vector<uint8_t> data(u.ByteSizeLong());
    u.SerializeToArray(data.data(), static_cast<int>(data.size()));
    return data;
}

class ChallengeModalRenderTest : public ::testing::Test {
protected:
    void SetUp() override {
        dbPath_ = "test_modal_render.sqlite3";
        std::remove(dbPath_.c_str());
        db_ = new Database(dbPath_);
        wrapper_ = new DatabaseWrapper(*db_, *db_);
        debate_ = new DebateWrapper(*wrapper_);
        ASSERT_TRUE(wrapper_->ensureAllTables());
        alice_ = wrapper_->users.createUser("alice", modalTestBytes(modalTestUser("alice")));
        bob_ = wrapper_->users.createUser("bob", modalTestBytes(modalTestUser("bob")));
        ASSERT_GT(alice_, 0);
        ASSERT_GT(bob_, 0);
        user::User a = modalTestUser("alice"); a.set_user_id(alice_);
        user::User b = modalTestUser("bob");   b.set_user_id(bob_);
        wrapper_->users.updateUserProtobuf(alice_, modalTestBytes(a));
        wrapper_->users.updateUserProtobuf(bob_, modalTestBytes(b));
    }
    void TearDown() override {
        delete debate_;
        delete wrapper_;
        delete db_;
        std::remove(dbPath_.c_str());
        std::remove((dbPath_ + "-wal").c_str());
        std::remove((dbPath_ + "-shm").c_str());
    }

    // The page a viewer would get, as text, so its ids and labels can be read.
    std::string render(int viewerId, const std::string& viewerName) {
        const std::vector<int> users = debate_->findUsersInDebate(1);
        const debate::Collection collection =
            BuildCollection::BuildForDebateAndUsers(1, users, *debate_);
        const rendering_info::FullDebateViewInfo info =
            FullDebatePageInfoParser::ParseFullDebateViewInfo(collection, viewerId, viewerName);
        VRUserDatabase userDb(*db_);
        // The page reads the current claim's sentence and the links under it
        // from the viewer's debating_info, which the response generator fills
        // per request -- the stored user carries only ids. Go through it, as
        // the server does, or the modal renders against an empty copy.
        const user::User stored = debate_->getUserProtobuf(viewerId);
        moderator_to_vr::ModeratorToVRMessage response;
        DebatePageResponseGenerator::BuildDebatePageResponse(response, viewerId, stored, *debate_);
        user::User viewer = stored;
        *viewer.mutable_engagement()->mutable_debating_info() =
            response.user().engagement().debating_info();
        return StepView::GenerateStepViewPage(info, collection, userDb, viewer).DebugString();
    }

    std::string dbPath_;
    Database* db_ = nullptr;
    DatabaseWrapper* wrapper_ = nullptr;
    DebateWrapper* debate_ = nullptr;
    int alice_ = 0;
    int bob_ = 0;
};

}  // namespace

TEST_F(ChallengeModalRenderTest, TheModalOpensForAClaimAndForAnArgumentAlike) {
    // Alice: a root with one piece of evidence under it.
    DebateHandler::AddDebate("Speed limits save lives", alice_, *debate_);
    MoveUserHandler::EnterDebate(1, alice_, *debate_);
    AddClaimHandler::OpenAddChildClaim(alice_, *debate_);
    AddClaimHandler::AddClaimUnderClaim({"Crash deaths fell 23 percent"}, {""}, "", alice_, *debate_);
    const std::vector<int> links = debate_->findLinksUnder(1);
    ASSERT_EQ(links.size(), 1u);

    // Bob arrives, standing on the root, with no modal anywhere.
    MoveUserHandler::EnterDebate(1, bob_, *debate_);
    MoveUserHandler::SelectClaim(1, bob_, *debate_);
    {
        const std::string page = render(bob_, "bob");
        EXPECT_NE(page.find("challengeClaimButton"), std::string::npos) << "the claim card's Challenge button";
        EXPECT_EQ(page.find("challengeSentenceInput"), std::string::npos) << "no modal before a click";
    }

    // The claim card's Challenge button: the modal, aimed at the claim.
    ChallengeHandler::StartChallengeClaim(bob_, *debate_);
    {
        const std::string page = render(bob_, "bob");
        EXPECT_NE(page.find("challengeSentenceInput"), std::string::npos) << "the modal is up";
        EXPECT_NE(page.find("Challenging: Speed limits save lives"), std::string::npos) << page.substr(0, 0);
        EXPECT_EQ(page.find("Challenging the argument"), std::string::npos);
        EXPECT_EQ(page.find("challengeTargetBox"), std::string::npos) << "the picker list is gone";
    }
    ChallengeHandler::CancelChallengeClaim(bob_, *debate_);
    EXPECT_EQ(render(bob_, "bob").find("challengeSentenceInput"), std::string::npos) << "cancel closes it";

    // The pill: select it, and the card is the argument's, same shape as a claim's.
    MoveUserHandler::SelectLink(links[0], bob_, *debate_);
    {
        const std::string page = render(bob_, "bob");
        EXPECT_NE(page.find("Selected Argument:"), std::string::npos);
        EXPECT_NE(page.find("Crash deaths fell 23 percent  --  for: Speed limits save lives"), std::string::npos);
        EXPECT_NE(page.find("challengeSelectedLinkButton_" + std::to_string(links[0])), std::string::npos)
            << "the argument card's Challenge button";
        EXPECT_EQ(page.find("challengeSentenceInput"), std::string::npos);
    }

    // The argument card's Challenge button: the modal, aimed at the argument.
    ChallengeHandler::AddLinkToBeChallenged(links[0], bob_, *debate_);
    {
        const std::string page = render(bob_, "bob");
        EXPECT_NE(page.find("challengeSentenceInput"), std::string::npos) << "the modal is up";
        EXPECT_NE(page.find("Challenging the argument for \\\"Speed limits save lives\\\", offered: Crash deaths fell 23 percent"),
                  std::string::npos) << "the one line names the argument";
        EXPECT_NE(page.find("Say why the evidence offered"), std::string::npos);
        EXPECT_EQ(page.find("challengeTargetBox"), std::string::npos);
    }
}
