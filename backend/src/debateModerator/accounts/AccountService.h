#pragma once

#include <cstdint>
#include <string>

#include "../../database/sqlite/Database.h"
#include "../../database/debate/DatabaseWrapper.h"

// Everything done to an ACCOUNT rather than to a debate: names, renames,
// closing an account, linking Google, and the two jobs the server runs on
// its own (the email-name migration at startup, the visitor sweep).
//
// Two user tables. The live one is USERS in users.sqlite3: every id in the
// debates refers to it, and both the moderator (DatabaseWrapper::users) and
// the virtual renderer (VRUserDatabase) read it. debates.sqlite3 still holds
// an older USERS table from before the split. Nothing reads it, but on a
// server that predates the split it holds the same people by name --
// Google users under their email -- so every rename and every deletion here
// is applied to its row of the same name too. When both are the same file
// (the tests) there is only the one table.
class AccountService {
public:
    AccountService(DatabaseWrapper& db, Database& usersDb, Database& debatesDb);

    // Whether `name` is in use by an account other than `exceptUserId`,
    // ignoring case.
    bool IsTaken(const std::string& name, int exceptUserId = 0);

    // Why `name` cannot be a NEW username ("" when it can): the rules in
    // utils/UsernamePolicy.h and uniqueness.
    std::string CheckNewName(const std::string& name, int exceptUserId = 0);

    // ---- Google ---------------------------------------------------------
    // The account linked to this Google account, or -1.
    int FindByGoogleSub(const std::string& sub);
    // An account Google made before accounts were linked by google_sub: no
    // link, no password, and the email as its stored email or (before the
    // migration) as its name. -1 when there is none.
    int FindUnclaimedByEmail(const std::string& email);
    // A free username for a new Google account: the email's local part made
    // to fit the rules, then bob, bob2, bob3...
    std::string UniqueNameFromEmail(const std::string& email);

    // ---- Changes a person makes --------------------------------------------
    // Renames `userId` to `newName`, checked against the rules. "" on
    // success, else why not. Renaming to the same name is a success.
    std::string Rename(int userId, const std::string& newName);
    // Closes the account: email, password, Google link, saved view state and
    // AI chat history are wiped; it is renamed "deleted-<id>" so it can never
    // be signed into again. Its claims stay (other people's debates hang off
    // them) and show as "Deleted account". False if there is nothing to close
    // (no such account, a visitor, already closed).
    bool Delete(int userId);

    // ---- Run by the server -------------------------------------------------
    // Google accounts used to be named by their email. Renames every account
    // whose name contains '@' and that is Google's (it has a google_sub, or
    // it has no password, which is how the old Google sign-in made them) to a
    // name from the email's local part, keeping the email in the private
    // `email` field so the next Google sign-in still finds it. In both user
    // tables. An '@' name with a password is a person's own choice of
    // sign-in name and is left alone, with a warning. Returns how many were
    // renamed. Safe to run on every start.
    int MigrateEmailUsernames();

    // Deletes visitor accounts that are more than `maxAgeSeconds` old by
    // joined_at (a visitor with no joined_at counts as old): their rows in
    // both user tables, chat history and any membership rows. Only accounts
    // that are visitor-shaped in every way -- the exact "visitor-<8 hex>"
    // name, no password, no Google link, no email -- and that never entered
    // anything (no claim as entrant or voice, no move, no debate) are
    // touched. Returns how many were deleted.
    int SweepVisitors(int64_t now, int64_t maxAgeSeconds = kVisitorLifetimeSeconds);

    static constexpr int64_t kVisitorLifetimeSeconds = 3LL * 24 * 60 * 60;

private:
    // The account's row in both tables and the name inside its record.
    bool renameEverywhere(int userId, const std::string& oldName, const std::string& newName);
    // The old table, when it is a different file and has a USERS table.
    bool hasLegacyTable();
    bool legacyNameTaken(const std::string& name);
    // Renames the old table's row called `oldName`, and puts `newName` in
    // its record; with `scrub`, the record keeps nothing else. False when
    // there is no such row or the new name is taken there.
    bool legacyRename(const std::string& oldName, const std::string& newName, bool scrub);
    bool legacyDelete(const std::string& name);

    DatabaseWrapper& db_;
    Database& usersDb_;
    Database& debatesDb_;
};
