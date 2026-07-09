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

    // Create moderator (uses existing DB)
    DebateModerator moderator;

    // Standalone DB handle wired to the same DB_PATH as moderator, used only to
    // reach a DebateWrapper for read-only collection building in tests
    // (DebateModerator no longer exposes its internal DebateWrapper).
    Database wrapperDb(db_path);
    DatabaseWrapper wrapperDbWrapper(wrapperDb);
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
    ui::Page page = StepView::GenerateStepViewPage(fullDebateInfo, collection, userDb);

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
