// test_summary_store.cc -- per-user AI summary storage
//
// Every bug in the summary feature so far was found by hand, in the browser,
// after the fact: a stale move-seq, a stored summary nothing ever looked up,
// a finished run nobody collected. None of them were exotic, and none of them
// would have survived a test. This file covers the storage layer, which is
// where the "looked it up with the wrong key" class of bug lives.
//
// The invariants worth pinning down are all about the UNIQUE key. One row per
// (user, debate, kind, claim) is the whole design: it is what keeps two users
// from overwriting each other, what makes a re-summarize replace instead of
// accumulate, and what lets several claims in one debate each keep a summary.

#include <gtest/gtest.h>

#include <cstdio>
#include <string>

#include "database/sqlite/Database.h"
#include "database/debate/DatabaseWrapper.h"

#ifdef _WIN32
#include <process.h>
#define GETPID _getpid
#else
#include <unistd.h>
#define GETPID getpid
#endif

class SummaryStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = std::string("test_summarydb_") + std::to_string(GETPID()) + ".sqlite3";
        std::remove(db_path_.c_str());
        db_ = new Database(db_path_);
        wrapper_ = new DatabaseWrapper(*db_, *db_);
        ASSERT_TRUE(wrapper_->ensureAllTables());
    }

    void TearDown() override {
        delete wrapper_;
        delete db_;
        std::remove(db_path_.c_str());
        std::remove((db_path_ + "-wal").c_str());
        std::remove((db_path_ + "-shm").c_str());
    }

    SummaryDatabase& store() { return wrapper_->summaries; }

    std::string db_path_;
    Database* db_ = nullptr;
    DatabaseWrapper* wrapper_ = nullptr;
};

TEST_F(SummaryStoreTest, SavesAndLoadsBackWhatWasStored) {
    ASSERT_TRUE(store().save(5, 20, "debate", 0, "the summary text", "", 42));

    const SummaryDatabase::Record record = store().load(5, 20, "debate", 0);
    ASSERT_TRUE(record.found);
    EXPECT_EQ(record.text, "the summary text");
    EXPECT_TRUE(record.error.empty());
    EXPECT_EQ(record.atMoveSeq, 42) << "the stamp is what staleness is computed from";
}

TEST_F(SummaryStoreTest, MissingSummaryIsNotFoundRatherThanEmpty) {
    const SummaryDatabase::Record record = store().load(5, 20, "debate", 0);
    EXPECT_FALSE(record.found)
        << "an absent summary and an empty one must be distinguishable -- the UI "
           "shows a prompt for one and a blank panel for the other";
}

// Re-summarizing replaces. Accumulating would mean load() had to pick a
// winner, and every read would depend on that choice being right.
TEST_F(SummaryStoreTest, ResummarizingReplacesRatherThanAccumulating) {
    ASSERT_TRUE(store().save(5, 20, "debate", 0, "first version", "", 10));
    ASSERT_TRUE(store().save(5, 20, "debate", 0, "second version", "", 11));

    const SummaryDatabase::Record record = store().load(5, 20, "debate", 0);
    ASSERT_TRUE(record.found);
    EXPECT_EQ(record.text, "second version");
    EXPECT_EQ(record.atMoveSeq, 11) << "the stamp must move with the text it describes";
}

// THE ISOLATION THE DESIGN EXISTS FOR. Summaries are per user precisely so
// that one person pressing Re-summarize cannot rewrite what another is
// reading.
TEST_F(SummaryStoreTest, OneUsersSummaryDoesNotTouchAnothers) {
    ASSERT_TRUE(store().save(5, 20, "debate", 0, "summary for user five", "", 10));
    ASSERT_TRUE(store().save(6, 20, "debate", 0, "summary for user six", "", 10));

    EXPECT_EQ(store().load(5, 20, "debate", 0).text, "summary for user five");
    EXPECT_EQ(store().load(6, 20, "debate", 0).text, "summary for user six");
}

// The bug this replaces: the proto held exactly one claim summary, so
// summarizing a second claim destroyed the first.
TEST_F(SummaryStoreTest, EachClaimKeepsItsOwnSummary) {
    ASSERT_TRUE(store().save(5, 20, "claim", 94, "about claim 94", "", 10));
    ASSERT_TRUE(store().save(5, 20, "claim", 110, "about claim 110", "", 10));

    EXPECT_EQ(store().load(5, 20, "claim", 94).text, "about claim 94");
    EXPECT_EQ(store().load(5, 20, "claim", 110).text, "about claim 110")
        << "summarizing a second claim must not evict the first";
}

TEST_F(SummaryStoreTest, DebateAndClaimSummariesCoexistForOneDebate) {
    ASSERT_TRUE(store().save(5, 20, "debate", 0, "whole debate", "", 10));
    ASSERT_TRUE(store().save(5, 20, "claim", 94, "one claim", "", 10));

    EXPECT_EQ(store().load(5, 20, "debate", 0).text, "whole debate");
    EXPECT_EQ(store().load(5, 20, "claim", 94).text, "one claim");
}

TEST_F(SummaryStoreTest, SummariesAreScopedToTheirDebate) {
    ASSERT_TRUE(store().save(5, 15, "debate", 0, "debate fifteen", "", 10));
    ASSERT_TRUE(store().save(5, 20, "debate", 0, "debate twenty", "", 10));

    EXPECT_EQ(store().load(5, 15, "debate", 0).text, "debate fifteen");
    EXPECT_EQ(store().load(5, 20, "debate", 0).text, "debate twenty");
}

// A failed run is stored too. Losing it would leave the panel offering to
// summarize as though nothing had been tried, hiding the reason.
TEST_F(SummaryStoreTest, AFailureIsStoredAndDistinguishableFromASummary) {
    ASSERT_TRUE(store().save(5, 20, "debate", 0, "", "could not reach the model", 10));

    const SummaryDatabase::Record record = store().load(5, 20, "debate", 0);
    ASSERT_TRUE(record.found);
    EXPECT_TRUE(record.text.empty());
    EXPECT_EQ(record.error, "could not reach the model");
}

TEST_F(SummaryStoreTest, RemoveDeletesOnlyTheTargetedSummary) {
    ASSERT_TRUE(store().save(5, 20, "debate", 0, "whole debate", "", 10));
    ASSERT_TRUE(store().save(5, 20, "claim", 94, "one claim", "", 10));
    ASSERT_TRUE(store().save(6, 20, "debate", 0, "another user", "", 10));

    ASSERT_TRUE(store().remove(5, 20, "claim", 94));

    EXPECT_FALSE(store().load(5, 20, "claim", 94).found);
    EXPECT_TRUE(store().load(5, 20, "debate", 0).found) << "dismiss must not clear the other panel";
    EXPECT_TRUE(store().load(6, 20, "debate", 0).found) << "or anyone else's";
}

TEST_F(SummaryStoreTest, RefusesToStoreASummaryWithNoOwnerOrDebate) {
    EXPECT_FALSE(store().save(0, 20, "debate", 0, "orphan", "", 1))
        << "a summary with no user could be served to anyone";
    EXPECT_FALSE(store().save(5, 0, "debate", 0, "orphan", "", 1))
        << "and one with no debate describes nothing";
}

// Summaries outliving the process is the entire point of storing them: a run
// costs tens of seconds of model time.
TEST_F(SummaryStoreTest, SummariesSurviveReopeningTheDatabase) {
    ASSERT_TRUE(store().save(5, 20, "debate", 0, "persisted across a restart", "", 7));

    delete wrapper_;
    delete db_;
    db_ = new Database(db_path_);
    wrapper_ = new DatabaseWrapper(*db_, *db_);
    ASSERT_TRUE(wrapper_->ensureAllTables());

    const SummaryDatabase::Record record = store().load(5, 20, "debate", 0);
    ASSERT_TRUE(record.found);
    EXPECT_EQ(record.text, "persisted across a restart");
    EXPECT_EQ(record.atMoveSeq, 7);
}
