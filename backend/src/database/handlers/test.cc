#include "DebateMembersDatabaseHandler.h"
#include <iostream>
#include <cassert>
#include <filesystem>

void testDebateMembersDatabase() {
    std::cout << "\n========================================\n";
    std::cout << "Testing DebateMembersDatabaseHandler\n";
    std::cout << "========================================\n\n";

    // Use a test database file
    std::string testDbPath = "test_debate_members.sqlite3";
    
    // Delete test database if it exists
    if (std::filesystem::exists(testDbPath)) {
        std::filesystem::remove(testDbPath);
        std::cout << "[TEST] Removed existing test database\n\n";
    }

    DebateMembersDatabaseHandler handler(testDbPath);

    // Test 1: Add members
    std::cout << "--- Test 1: Add Members ---\n";
    int id1 = handler.addMember("debate1", "alice");
    int id2 = handler.addMember("debate1", "bob");
    int id3 = handler.addMember("debate2", "alice");
    int id4 = handler.addMember("debate2", "charlie");
    int id5 = handler.addMember("debate3", "bob");
    
    assert(id1 > 0);
    assert(id2 > 0);
    assert(id3 > 0);
    assert(id4 > 0);
    assert(id5 > 0);
    std::cout << "[TEST PASSED] All members added successfully\n\n";

    // Test 2: Get debates for a user
    std::cout << "--- Test 2: Get Debates for User ---\n";
    auto aliceDebates = handler.getDebateIdsForUser("alice");
    assert(aliceDebates.size() == 2);
    std::cout << "Alice's debates: ";
    for (const auto& id : aliceDebates) std::cout << id << " ";
    std::cout << "\n";
    
    auto bobDebates = handler.getDebateIdsForUser("bob");
    assert(bobDebates.size() == 2);
    std::cout << "Bob's debates: ";
    for (const auto& id : bobDebates) std::cout << id << " ";
    std::cout << "\n";
    
    auto charlieDebates = handler.getDebateIdsForUser("charlie");
    assert(charlieDebates.size() == 1);
    std::cout << "Charlie's debates: ";
    for (const auto& id : charlieDebates) std::cout << id << " ";
    std::cout << "\n";
    std::cout << "[TEST PASSED] User debate queries work correctly\n\n";

    // Test 3: Get users for a debate
    std::cout << "--- Test 3: Get Users for Debate ---\n";
    auto debate1Users = handler.getUserIdsForDebate("debate1");
    assert(debate1Users.size() == 2);
    std::cout << "Debate1 users: ";
    for (const auto& user : debate1Users) std::cout << user << " ";
    std::cout << "\n";
    
    auto debate2Users = handler.getUserIdsForDebate("debate2");
    assert(debate2Users.size() == 2);
    std::cout << "Debate2 users: ";
    for (const auto& user : debate2Users) std::cout << user << " ";
    std::cout << "\n";
    
    auto debate3Users = handler.getUserIdsForDebate("debate3");
    assert(debate3Users.size() == 1);
    std::cout << "Debate3 users: ";
    for (const auto& user : debate3Users) std::cout << user << " ";
    std::cout << "\n";
    std::cout << "[TEST PASSED] Debate user queries work correctly\n\n";

    // Test 4: Check membership
    std::cout << "--- Test 4: Check Membership ---\n";
    assert(handler.isMember("debate1", "alice") == true);
    assert(handler.isMember("debate1", "bob") == true);
    assert(handler.isMember("debate1", "charlie") == false);
    assert(handler.isMember("debate2", "charlie") == true);
    assert(handler.isMember("debate3", "alice") == false);
    std::cout << "[TEST PASSED] Membership checks work correctly\n\n";

    // Test 5: Remove a specific member
    std::cout << "--- Test 5: Remove Member ---\n";
    bool removed = handler.removeMember("debate1", "alice");
    assert(removed == true);
    assert(handler.isMember("debate1", "alice") == false);
    auto debate1UsersAfter = handler.getUserIdsForDebate("debate1");
    assert(debate1UsersAfter.size() == 1);
    std::cout << "[TEST PASSED] Member removal works correctly\n\n";

    // Test 6: Remove all members from a debate
    std::cout << "--- Test 6: Remove All Members from Debate ---\n";
    bool removedAll = handler.removeAllMembersFromDebate("debate2");
    assert(removedAll == true);
    auto debate2UsersAfter = handler.getUserIdsForDebate("debate2");
    assert(debate2UsersAfter.size() == 0);
    std::cout << "[TEST PASSED] Removing all members from debate works\n\n";

    // Test 7: Remove user from all debates
    std::cout << "--- Test 7: Remove User from All Debates ---\n";
    bool removedUser = handler.removeUserFromAllDebates("bob");
    assert(removedUser == true);
    auto bobDebatesAfter = handler.getDebateIdsForUser("bob");
    assert(bobDebatesAfter.size() == 0);
    auto debate1Final = handler.getUserIdsForDebate("debate1");
    assert(debate1Final.size() == 0); // bob was the only one left
    auto debate3Final = handler.getUserIdsForDebate("debate3");
    assert(debate3Final.size() == 0); // bob was in debate3
    std::cout << "[TEST PASSED] Removing user from all debates works\n\n";

    // Clean up test database
    std::filesystem::remove(testDbPath);
    std::cout << "[TEST] Cleaned up test database\n";

    std::cout << "\n========================================\n";
    std::cout << "ALL TESTS PASSED! ✓\n";
    std::cout << "========================================\n\n";
}

int main() {
    try {
        testDebateMembersDatabase();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
