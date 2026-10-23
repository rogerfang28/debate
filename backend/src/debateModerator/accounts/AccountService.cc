#include "AccountService.h"

#include <filesystem>
#include <set>
#include <vector>

#include "debate.pb.h"
#include "user.pb.h"
#include "../../utils/AnonymousUser.h"
#include "../../utils/Log.h"
#include "../../utils/UsernamePolicy.h"

namespace {

bool parseUser(const std::vector<uint8_t>& blob, user::User* out) {
    return !blob.empty() && out->ParseFromArray(blob.data(), static_cast<int>(blob.size()));
}

std::vector<uint8_t> serialize(const user::User& u) {
    const std::string bytes = u.SerializeAsString();
    return std::vector<uint8_t>(bytes.begin(), bytes.end());
}

// Every value of one integer column, for the sweep's "was this id ever
// used" checks. Empty if the table does not exist.
std::set<int> intColumn(Database& db, const std::string& sql) {
    std::set<int> out;
    sqlite3_stmt* stmt = db.prepare(sql);
    if (!stmt) return out;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        out.insert(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return out;
}

bool samePath(const std::string& a, const std::string& b) {
    std::error_code ec1, ec2;
    const auto pa = std::filesystem::weakly_canonical(a, ec1);
    const auto pb = std::filesystem::weakly_canonical(b, ec2);
    if (ec1 || ec2) return a == b;
    return pa == pb;
}

}  // namespace

AccountService::AccountService(DatabaseWrapper& db, Database& usersDb, Database& debatesDb)
    : db_(db), usersDb_(usersDb), debatesDb_(debatesDb) {}

bool AccountService::IsTaken(const std::string& name, int exceptUserId) {
    return db_.users.findIdIgnoringCase(name, exceptUserId) != -1;
}

std::string AccountService::CheckNewName(const std::string& name, int exceptUserId) {
    return username_policy::Check(name, [&](const std::string& n) { return IsTaken(n, exceptUserId); });
}

// ---- Google -----------------------------------------------------------------

int AccountService::FindByGoogleSub(const std::string& sub) {
    if (sub.empty()) return -1;
    for (const auto& [id, name] : db_.users.listUsers()) {
        user::User u;
        if (parseUser(db_.users.getUserProtobuf(id), &u) && u.google_sub() == sub) return id;
    }
    return -1;
}

int AccountService::FindUnclaimedByEmail(const std::string& email) {
    if (email.empty()) return -1;
    const std::string wanted = username_policy::Lower(email);
    for (const auto& [id, name] : db_.users.listUsers()) {
        if (anonymous_user::IsAnonymousUsername(name) || username_policy::IsDeletedName(name)) continue;
        user::User u;
        if (!parseUser(db_.users.getUserProtobuf(id), &u)) continue;
        if (!u.google_sub().empty() || !u.password_hash().empty()) continue;
        if (username_policy::Lower(u.email()) == wanted || username_policy::Lower(name) == wanted) return id;
    }
    return -1;
}

std::string AccountService::UniqueNameFromEmail(const std::string& email) {
    return username_policy::MakeUnique(username_policy::BaseFromEmail(email), [&](const std::string& n) {
        return IsTaken(n) || legacyNameTaken(n);
    });
}

// ---- Changes a person makes -------------------------------------------------

std::string AccountService::Rename(int userId, const std::string& newName) {
    const std::string oldName = db_.users.getUsername(userId);
    if (oldName.empty()) return "That account doesn't exist.";
    if (anonymous_user::IsAnonymousUsername(oldName) || username_policy::IsDeletedName(oldName)) {
        return "This account can't be renamed.";
    }
    if (newName == oldName) return "";
    const std::string error = CheckNewName(newName, userId);
    if (!error.empty()) return error;
    if (!renameEverywhere(userId, oldName, newName)) {
        return "That username couldn't be saved. Please try again.";
    }
    Log::info("[Accounts] user_id=" + std::to_string(userId) + " renamed '" + oldName + "' -> '" + newName + "'");
    return "";
}

bool AccountService::Delete(int userId) {
    const std::string oldName = db_.users.getUsername(userId);
    if (oldName.empty() || anonymous_user::IsAnonymousUsername(oldName) ||
        username_policy::IsDeletedName(oldName)) {
        return false;
    }
    const std::string closedName = username_policy::DeletedName(userId);
    if (IsTaken(closedName, userId)) {
        Log::error("[Accounts] Cannot close user_id=" + std::to_string(userId) + ": the name " + closedName +
                   " is already in use.");
        return false;
    }

    // A fresh record: nothing of the old one survives but the id.
    user::User closed;
    closed.set_user_id(userId);
    closed.set_username(closedName);
    closed.mutable_engagement()->set_current_action(user_engagement::ACTION_HOME);
    if (!db_.users.updateUsername(userId, closedName) ||
        !db_.users.updateUserProtobuf(userId, serialize(closed))) {
        Log::error("[Accounts] Closing user_id=" + std::to_string(userId) + " failed to write the record.");
        return false;
    }
    db_.chats.clearAllForUser(userId);
    if (hasLegacyTable()) legacyRename(oldName, closedName, /*scrub=*/true);
    // Membership rows stay: a debate's page is built from its members'
    // claims, and the claims stay.
    Log::info("[Accounts] user_id=" + std::to_string(userId) + " closed their account; now " + closedName);
    return true;
}

// ---- Run by the server ------------------------------------------------------

int AccountService::MigrateEmailUsernames() {
    int renamed = 0;
    for (const auto& [id, name] : db_.users.listUsers()) {
        if (name.find('@') == std::string::npos) continue;
        user::User u;
        if (!parseUser(db_.users.getUserProtobuf(id), &u)) {
            Log::warn("[Accounts] Migration: user_id=" + std::to_string(id) +
                      " is named by an email but its record does not parse; left alone.");
            continue;
        }
        const bool google = !u.google_sub().empty();
        if (!google && !u.password_hash().empty()) {
            Log::warn("[Accounts] Migration: user_id=" + std::to_string(id) +
                      " is named by an email but signs in with a password; left alone.");
            continue;
        }
        // The address moves to the private field, where the next Google
        // sign-in looks for it.
        if (u.email().empty()) {
            u.set_email(name);
            db_.users.updateUserProtobuf(id, serialize(u));
        }
        const std::string newName = UniqueNameFromEmail(name);
        if (!renameEverywhere(id, name, newName)) {
            Log::error("[Accounts] Migration: renaming user_id=" + std::to_string(id) + " failed.");
            continue;
        }
        if (hasLegacyTable()) legacyRename(newName, newName, /*scrub=*/true);
        Log::info("[Accounts] Migration: user_id=" + std::to_string(id) + " renamed from an email address to '" +
                  newName + "' (" + (google ? "Google account" : "Google account from before linking") + ")");
        ++renamed;
    }

    // Rows of the old table still named by an email (an account renamed on
    // an earlier start whose old row was not, or one only the old table has).
    int legacyRenamed = 0;
    if (hasLegacyTable()) {
        std::vector<std::string> emailRows;
        sqlite3_stmt* stmt = debatesDb_.prepare("SELECT USERNAME FROM USERS WHERE USERNAME LIKE '%@%';");
        if (stmt) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const unsigned char* text = sqlite3_column_text(stmt, 0);
                if (text) emailRows.emplace_back(reinterpret_cast<const char*>(text));
            }
            sqlite3_finalize(stmt);
        }
        for (const std::string& email : emailRows) {
            // The live account this address belongs to, if any, lends its name.
            std::string newName;
            const std::string wanted = username_policy::Lower(email);
            for (const auto& [id, name] : db_.users.listUsers()) {
                user::User u;
                if (parseUser(db_.users.getUserProtobuf(id), &u) && username_policy::Lower(u.email()) == wanted &&
                    name.find('@') == std::string::npos) {
                    newName = name;
                    break;
                }
            }
            if (newName.empty() || legacyNameTaken(newName)) newName = UniqueNameFromEmail(email);
            if (legacyRename(email, newName, /*scrub=*/true)) {
                Log::info("[Accounts] Migration: old users table row renamed from an email address to '" +
                          newName + "'");
                ++legacyRenamed;
            }
        }
    }
    Log::info("[Accounts] Email-name migration: " + std::to_string(renamed) + " account(s) renamed" +
              (legacyRenamed > 0 ? ", " + std::to_string(legacyRenamed) + " more row(s) in the old users table"
                                 : std::string()) + ".");
    return renamed;
}

int AccountService::SweepVisitors(int64_t now, int64_t maxAgeSeconds) {
    struct Candidate { int id; std::string name; };
    std::vector<Candidate> candidates;
    for (const auto& [id, name] : db_.users.listUsers()) {
        if (!anonymous_user::IsWellFormedVisitorName(name)) continue;
        user::User u;
        const std::vector<uint8_t> blob = db_.users.getUserProtobuf(id);
        if (!blob.empty() && !parseUser(blob, &u)) continue;
        if (!u.password_hash().empty() || !u.google_sub().empty() || !u.email().empty()) {
            Log::warn("[Accounts] Visitor sweep: user_id=" + std::to_string(id) +
                      " has a visitor's name but a password, Google link or email; kept.");
            continue;
        }
        if (u.joined_at() > 0 && now - u.joined_at() <= maxAgeSeconds) continue;
        candidates.push_back({id, name});
    }

    int removed = 0;
    int kept = 0;
    if (!candidates.empty()) {
        // Anyone who ever entered anything stays: a debate is built from its
        // members' claims, and a move or a voice points at its account.
        std::set<int> used = intColumn(debatesDb_, "SELECT DISTINCT CREATOR_ID FROM STATEMENTS;");
        for (int id : intColumn(debatesDb_, "SELECT DISTINCT ACTOR_ID FROM MOVES;")) used.insert(id);
        for (int id : intColumn(debatesDb_, "SELECT DISTINCT USER_ID FROM DEBATE;")) used.insert(id);
        if (sqlite3_stmt* stmt = debatesDb_.prepare("SELECT STATEMENT_DATA FROM STATEMENTS;")) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const void* data = sqlite3_column_blob(stmt, 0);
                const int size = sqlite3_column_bytes(stmt, 0);
                debate::Claim claim;
                if (data && size > 0 && claim.ParseFromArray(data, size) && claim.voice_id() > 0) {
                    used.insert(claim.voice_id());
                }
            }
            sqlite3_finalize(stmt);
        }

        const bool legacy = hasLegacyTable();
        for (const Candidate& c : candidates) {
            if (used.count(c.id)) {
                ++kept;
                Log::warn("[Accounts] Visitor sweep: user_id=" + std::to_string(c.id) +
                          " entered something in a debate; kept.");
                continue;
            }
            db_.chats.clearAllForUser(c.id);
            db_.debateMembers.removeUserFromAllDebates(c.id);
            if (!db_.users.deleteUser(c.id)) {
                Log::error("[Accounts] Visitor sweep: could not delete user_id=" + std::to_string(c.id));
                continue;
            }
            if (legacy) legacyDelete(c.name);
            ++removed;
        }
    }
    Log::info("[Accounts] Visitor sweep: removed " + std::to_string(removed) + " visitor account(s) older than " +
              std::to_string(maxAgeSeconds / 86400) + " days" +
              (kept > 0 ? " (" + std::to_string(kept) + " kept: they entered something)" : std::string()) + ".");
    return removed;
}

// ---- Private ----------------------------------------------------------------

bool AccountService::renameEverywhere(int userId, const std::string& oldName, const std::string& newName) {
    if (!db_.users.updateUsername(userId, newName)) return false;
    user::User u;
    if (parseUser(db_.users.getUserProtobuf(userId), &u)) {
        u.set_username(newName);
        db_.users.updateUserProtobuf(userId, serialize(u));
    }
    if (hasLegacyTable()) legacyRename(oldName, newName, /*scrub=*/false);
    return true;
}

bool AccountService::hasLegacyTable() {
    if (&usersDb_ == &debatesDb_ || samePath(usersDb_.path(), debatesDb_.path())) return false;
    sqlite3_stmt* stmt = debatesDb_.prepare("SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = 'USERS';");
    if (!stmt) return false;
    const bool exists = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return exists;
}

bool AccountService::legacyNameTaken(const std::string& name) {
    if (!hasLegacyTable()) return false;
    sqlite3_stmt* stmt = debatesDb_.prepare("SELECT 1 FROM USERS WHERE USERNAME = ? COLLATE NOCASE;");
    if (!stmt) return false;
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    const bool taken = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return taken;
}

bool AccountService::legacyRename(const std::string& oldName, const std::string& newName, bool scrub) {
    int rowId = -1;
    std::vector<uint8_t> blob;
    if (sqlite3_stmt* stmt = debatesDb_.prepare("SELECT ID, USER_DATA FROM USERS WHERE USERNAME = ?;")) {
        sqlite3_bind_text(stmt, 1, oldName.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            rowId = sqlite3_column_int(stmt, 0);
            const void* data = sqlite3_column_blob(stmt, 1);
            const int size = sqlite3_column_bytes(stmt, 1);
            if (data && size > 0) blob.assign(static_cast<const uint8_t*>(data), static_cast<const uint8_t*>(data) + size);
        }
        sqlite3_finalize(stmt);
    }
    if (rowId < 0) return false;
    if (newName != oldName) {
        // Taken by another row of the old table: leave this one as it is.
        sqlite3_stmt* check = debatesDb_.prepare("SELECT 1 FROM USERS WHERE USERNAME = ? AND ID != ?;");
        if (!check) return false;
        sqlite3_bind_text(check, 1, newName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(check, 2, rowId);
        const bool taken = sqlite3_step(check) == SQLITE_ROW;
        sqlite3_finalize(check);
        if (taken) {
            Log::warn("[Accounts] Old users table: '" + newName + "' is taken there; its row was not renamed.");
            return false;
        }
    }

    user::User u;
    if (!scrub) parseUser(blob, &u);
    if (scrub) {
        // Nothing but the name: the old table's records are read by nothing,
        // and an email or a Google link has no business staying in them.
        user::User old;
        if (parseUser(blob, &old)) u.set_user_id(old.user_id());
    }
    u.set_username(newName);
    const std::vector<uint8_t> data = serialize(u);

    sqlite3_stmt* stmt = debatesDb_.prepare("UPDATE USERS SET USERNAME = ?, USER_DATA = ? WHERE ID = ?;");
    if (!stmt) return false;
    sqlite3_bind_text(stmt, 1, newName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 2, data.data(), static_cast<int>(data.size()), SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, rowId);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool AccountService::legacyDelete(const std::string& name) {
    if (!anonymous_user::IsWellFormedVisitorName(name)) return false;  // visitors only
    sqlite3_stmt* stmt = debatesDb_.prepare("DELETE FROM USERS WHERE USERNAME = ?;");
    if (!stmt) return false;
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}
