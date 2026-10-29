#pragma once

#include <string>

#include "user_engagement.pb.h"

class DebateModerator;

// "Import debate", on an admin's own profile: paste a scenario file or an
// outline, Preview it, Publish it.
//
// The moderator lets only admins (utils/Admins.h) reach these; nothing here
// checks again. Preview writes nothing but the admin's own view state. Publish
// replays the steps through scenario::Importer -- the same replay the
// tutorials use -- as the admin, so an imported debate is built by the same
// events as a typed one.
namespace ImportHandler {

// Shows the import view on the admin's own profile, empty.
void Open(int user_id, DebateModerator& moderator);

// Leaves the import view for the profile, forgetting the text and preview.
void Close(int user_id, DebateModerator& moderator);

// Reads `text` and stores what it found (and the text) on the admin's view.
void Preview(const std::string& text, int user_id, DebateModerator& moderator);

// Builds the debate the stored preview describes and takes the admin into it.
// Publishes only when `text` is exactly what was previewed and had no errors;
// otherwise it previews `text` again and says why nothing was published. A
// failure part-way is reported on the import view, naming the step and any
// partial debate.
void Publish(const std::string& text, int user_id, DebateModerator& moderator);

// What Preview would show for `text`, imported by `admin_name`: the reading
// plus which voices are new accounts. Reads the users table, writes nothing.
user_engagement::ImportPreview Check(const std::string& text, const std::string& admin_name,
                                     DebateModerator& moderator);

}  // namespace ImportHandler
