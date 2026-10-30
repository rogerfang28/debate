#include "ActivityDatabase.h"
#include "../../utils/Log.h"

#include <cstdio>
#include <ctime>
#include <utility>

namespace {

// A MOVES.CREATED_AT ("2026-10-29T14:03:00.123Z", or the plain
// "2026-10-29 14:03:00" of older rows) as Unix milliseconds, in SQL.
const char* kMoveMs = "CAST(ROUND((julianday(m.CREATED_AT) - 2440587.5) * 86400000.0) AS INTEGER)";

// The moves that are a person's action, as activity kinds. Every support
// writes an ASSERT of each new claim and one of the relation; every
// challenge an ASSERT of each claim and an OPPOSE. So a support is its
// relation's ASSERT, a challenge its OPPOSE, and the only claim ASSERT that
// stands alone is a debate's first move: its root, when it was started.
const char* kMoveKind =
    "CASE m.TYPE WHEN 'ACCEPT' THEN 'agree' WHEN 'UNACCEPT' THEN 'withdraw' WHEN 'OPPOSE' THEN 'challenge' "
    "WHEN 'CONCEDE' THEN 'concede' WHEN 'RETRACT' THEN 'delete' "
    "ELSE CASE WHEN m.TARGET_TYPE = 'relation' THEN 'support' ELSE 'new_debate' END END";
const char* kMoveIsAction =
    "(m.TYPE IN ('ACCEPT','UNACCEPT','OPPOSE','CONCEDE','RETRACT') OR "
    "(m.TYPE = 'ASSERT' AND (m.TARGET_TYPE = 'relation' OR m.SEQ = 1)))";
// The claim it is about: what was agreed with, challenged, conceded or
// deleted; for a support, the claim supported.
const char* kMoveClaim =
    "CASE WHEN m.TYPE = 'ASSERT' AND m.TARGET_TYPE = 'relation' "
    "THEN (CASE WHEN json_valid(m.PAYLOAD) THEN IFNULL(json_extract(m.PAYLOAD, '$.from_claim_id'), 0) ELSE 0 END) "
    "WHEN m.TARGET_TYPE = 'claim' THEN m.TARGET_ID "
    "WHEN m.TYPE = 'OPPOSE' "
    "THEN (CASE WHEN json_valid(m.PAYLOAD) THEN IFNULL(json_extract(m.PAYLOAD, '$.challenging_claim_id'), 0) ELSE 0 END) "
    "ELSE 0 END";

// An admin's import replays every claim as the admin, just before its
// "import" row is written. Those moves are the import, already listed once:
// they are left out rather than counted as hundreds of actions.
std::string notImported() {
    return std::string("NOT EXISTS (SELECT 1 FROM ACTIVITY i WHERE i.KIND = 'import' AND i.DEBATE_ID = m.DEBATE_ID "
                       "AND i.USER_ID = m.ACTOR_ID AND i.AT_MS >= ") + kMoveMs + ")";
}

std::string moveColumns() {
    return std::string(kMoveMs) + " AS AT_MS, m.ACTOR_ID AS USER_ID, '' AS USERNAME, 1 AS IS_MEMBER, " +
           kMoveKind + " AS KIND, m.DEBATE_ID AS DEBATE_ID, " + kMoveClaim + " AS CLAIM_ID, '' AS DETAIL";
}

// Every row from ?1 (ms) / ?2 (the same time as CREATED_AT text) on.
std::string eventsSince() {
    return "WITH EV AS ("
           "SELECT AT_MS, USER_ID, USERNAME, IS_MEMBER, KIND, DEBATE_ID, CLAIM_ID, DETAIL "
           "FROM ACTIVITY WHERE AT_MS >= ?1 "
           "UNION ALL SELECT " + moveColumns() + " FROM MOVES m WHERE m.CREATED_AT >= ?2 AND " +
           kMoveIsAction + " AND " + notImported() + ") ";
}

const char* kTotals =
    "COUNT(DISTINCT CASE WHEN IS_MEMBER = 0 THEN USER_ID END), "
    "COUNT(DISTINCT CASE WHEN IS_MEMBER = 1 THEN USER_ID END), "
    "IFNULL(SUM(KIND = 'sign_in'), 0), IFNULL(SUM(KIND = 'sign_up'), 0), "
    "IFNULL(SUM(KIND IN ('new_debate','support','challenge','agree','withdraw','concede','delete')), 0), "
    "IFNULL(SUM(KIND = 'agree'), 0), IFNULL(SUM(KIND = 'challenge'), 0), "
    "IFNULL(SUM(KIND IN ('support','new_debate')), 0), IFNULL(SUM(KIND = 'open_debate'), 0)";

ActivityDatabase::Totals readTotals(sqlite3_stmt* stmt, int first) {
    ActivityDatabase::Totals t;
    t.visitors = sqlite3_column_int(stmt, first + 0);
    t.members = sqlite3_column_int(stmt, first + 1);
    t.signIns = sqlite3_column_int(stmt, first + 2);
    t.newAccounts = sqlite3_column_int(stmt, first + 3);
    t.actions = sqlite3_column_int(stmt, first + 4);
    t.agrees = sqlite3_column_int(stmt, first + 5);
    t.challenges = sqlite3_column_int(stmt, first + 6);
    t.claims = sqlite3_column_int(stmt, first + 7);
    t.debatesOpened = sqlite3_column_int(stmt, first + 8);
    return t;
}

std::string columnText(sqlite3_stmt* stmt, int i) {
    const unsigned char* t = sqlite3_column_text(stmt, i);
    return t ? reinterpret_cast<const char*>(t) : "";
}

void bindSince(sqlite3_stmt* stmt, int64_t sinceMs) {
    const std::string iso = ActivityDatabase::IsoFromMs(sinceMs);
    sqlite3_bind_int64(stmt, 1, sinceMs);
    sqlite3_bind_text(stmt, 2, iso.c_str(), -1, SQLITE_TRANSIENT);
}

}  // namespace

std::string ActivityDatabase::IsoFromMs(int64_t ms) {
    if (ms < 0) ms = 0;
    const std::time_t secs = static_cast<std::time_t>(ms / 1000);
    std::tm parts{};
#ifdef _WIN32
    gmtime_s(&parts, &secs);
#else
    gmtime_r(&secs, &parts);
#endif
    char buf[40];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", parts.tm_year + 1900, parts.tm_mon + 1,
                  parts.tm_mday, parts.tm_hour, parts.tm_min, parts.tm_sec, static_cast<int>(ms % 1000));
    return buf;
}

bool ActivityDatabase::ensureTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS ACTIVITY (
            ID        INTEGER PRIMARY KEY AUTOINCREMENT,
            AT_MS     INTEGER NOT NULL,
            USER_ID   INTEGER NOT NULL,
            USERNAME  TEXT NOT NULL DEFAULT '',
            IS_MEMBER INTEGER NOT NULL DEFAULT 0,
            KIND      TEXT NOT NULL,
            DEBATE_ID INTEGER NOT NULL DEFAULT 0,
            CLAIM_ID  INTEGER NOT NULL DEFAULT 0,
            DETAIL    TEXT NOT NULL DEFAULT ''
        );
    )";
    return db_.execute(sql) &&
           db_.execute("CREATE INDEX IF NOT EXISTS IDX_ACTIVITY_AT ON ACTIVITY(AT_MS);") &&
           db_.execute("CREATE INDEX IF NOT EXISTS IDX_ACTIVITY_KIND_AT ON ACTIVITY(KIND, AT_MS);") &&
           db_.execute("CREATE INDEX IF NOT EXISTS IDX_ACTIVITY_USER_AT ON ACTIVITY(USER_ID, AT_MS);") &&
           db_.execute("CREATE INDEX IF NOT EXISTS IDX_MOVES_CREATED_AT ON MOVES(CREATED_AT);");
}

bool ActivityDatabase::insert(const Row& row) {
    sqlite3_stmt* stmt = db_.prepare(
        "INSERT INTO ACTIVITY (AT_MS, USER_ID, USERNAME, IS_MEMBER, KIND, DEBATE_ID, CLAIM_ID, DETAIL) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?);");
    if (stmt == nullptr) {
        Log::warn("[ActivityDatabase] Could not prepare the insert for '" + row.kind + "'");
        return false;
    }
    sqlite3_bind_int64(stmt, 1, row.atMs);
    sqlite3_bind_int(stmt, 2, row.userId);
    sqlite3_bind_text(stmt, 3, row.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, row.member ? 1 : 0);
    sqlite3_bind_text(stmt, 5, row.kind.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, row.debateId);
    sqlite3_bind_int(stmt, 7, row.claimId);
    sqlite3_bind_text(stmt, 8, row.detail.c_str(), -1, SQLITE_TRANSIENT);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    if (!ok) {
        Log::warn("[ActivityDatabase] Could not record '" + row.kind + "' for user " + std::to_string(row.userId));
    }
    return ok;
}

bool ActivityDatabase::hasRowSince(int userId, int64_t sinceMs) {
    sqlite3_stmt* stmt = db_.prepare("SELECT 1 FROM ACTIVITY WHERE USER_ID = ? AND AT_MS >= ? LIMIT 1;");
    if (stmt == nullptr) return false;
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int64(stmt, 2, sinceMs);
    const bool found = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return found;
}

int ActivityDatabase::deleteBefore(int64_t cutoffMs) {
    sqlite3_stmt* stmt = db_.prepare("DELETE FROM ACTIVITY WHERE AT_MS < ?;");
    if (stmt == nullptr) return 0;
    sqlite3_bind_int64(stmt, 1, cutoffMs);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok ? sqlite3_changes(db_.handle()) : 0;
}

bool ActivityDatabase::renameUser(int userId, const std::string& newName) {
    sqlite3_stmt* stmt = db_.prepare("UPDATE ACTIVITY SET USERNAME = ? WHERE USER_ID = ?;");
    if (stmt == nullptr) return false;
    sqlite3_bind_text(stmt, 1, newName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, userId);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

ActivityDatabase::Totals ActivityDatabase::totalsSince(int64_t sinceMs) {
    Totals totals;
    sqlite3_stmt* stmt = db_.prepare(eventsSince() + "SELECT " + kTotals + " FROM EV;");
    if (stmt == nullptr) return totals;
    bindSince(stmt, sinceMs);
    if (sqlite3_step(stmt) == SQLITE_ROW) totals = readTotals(stmt, 0);
    sqlite3_finalize(stmt);
    return totals;
}

std::vector<ActivityDatabase::Day> ActivityDatabase::daysSince(int64_t sinceMs) {
    std::vector<Day> days;
    sqlite3_stmt* stmt = db_.prepare(eventsSince() + "SELECT date(AT_MS / 1000, 'unixepoch') AS D, " + kTotals +
                                     " FROM EV GROUP BY D ORDER BY D DESC;");
    if (stmt == nullptr) return days;
    bindSince(stmt, sinceMs);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        days.push_back({columnText(stmt, 0), readTotals(stmt, 1)});
    }
    sqlite3_finalize(stmt);
    return days;
}

std::vector<ActivityDatabase::TopDebate> ActivityDatabase::mostOpenedSince(int64_t sinceMs, int limit) {
    std::vector<TopDebate> top;
    sqlite3_stmt* stmt = db_.prepare(
        "SELECT DEBATE_ID, COUNT(*) AS N, COUNT(DISTINCT USER_ID) AS P FROM ACTIVITY "
        "WHERE KIND = 'open_debate' AND AT_MS >= ? AND DEBATE_ID > 0 "
        "GROUP BY DEBATE_ID ORDER BY N DESC, P DESC, DEBATE_ID DESC LIMIT ?;");
    if (stmt == nullptr) return top;
    sqlite3_bind_int64(stmt, 1, sinceMs);
    sqlite3_bind_int(stmt, 2, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        top.push_back({sqlite3_column_int(stmt, 0), sqlite3_column_int(stmt, 1), sqlite3_column_int(stmt, 2)});
    }
    sqlite3_finalize(stmt);
    return top;
}

std::vector<ActivityDatabase::Row> ActivityDatabase::recent(const std::string& filter, int limit, int64_t sinceMs) {
    std::vector<Row> rows;
    const bool visitorsOnly = filter == "visitors";
    const bool membersOnly = filter == "members";
    std::string sql =
        "SELECT * FROM (SELECT AT_MS, USER_ID, USERNAME, IS_MEMBER, KIND, DEBATE_ID, CLAIM_ID, DETAIL "
        "FROM ACTIVITY WHERE AT_MS >= ?2" +
        std::string(visitorsOnly ? " AND IS_MEMBER = 0" : membersOnly ? " AND IS_MEMBER = 1" : "") +
        " ORDER BY AT_MS DESC, ID DESC LIMIT ?1)";
    // A visitor cannot act in a debate, so the move log has none of theirs.
    if (!visitorsOnly) {
        sql += " UNION ALL SELECT * FROM (SELECT " + moveColumns() + " FROM MOVES m WHERE m.CREATED_AT >= ?3 AND " + kMoveIsAction +
               " AND " + notImported() + " ORDER BY m.CREATED_AT DESC, m.ID DESC LIMIT ?1)";
    }
    sql += " ORDER BY AT_MS DESC LIMIT ?1;";
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (stmt == nullptr) return rows;
    const std::string iso = IsoFromMs(sinceMs);
    sqlite3_bind_int(stmt, 1, limit);
    sqlite3_bind_int64(stmt, 2, sinceMs);
    if (!visitorsOnly) sqlite3_bind_text(stmt, 3, iso.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Row r;
        r.atMs = sqlite3_column_int64(stmt, 0);
        r.userId = sqlite3_column_int(stmt, 1);
        r.username = columnText(stmt, 2);
        r.member = sqlite3_column_int(stmt, 3) != 0;
        r.kind = columnText(stmt, 4);
        r.debateId = sqlite3_column_int(stmt, 5);
        r.claimId = sqlite3_column_int(stmt, 6);
        r.detail = columnText(stmt, 7);
        rows.push_back(std::move(r));
    }
    sqlite3_finalize(stmt);
    return rows;
}
