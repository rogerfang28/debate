# Test CMakeLists.txt for Debate Tool — tests branch
# Build: cd test && cmake -S . -B build -G Ninja && cmake --build build -j
# Test:  cd build && ctest --output-on-failure -j4

cmake_minimum_required(VERSION 3.20)
project(DebateTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- vcpkg paths (repo root is parent of test/) ---
set(REPO_ROOT "${CMAKE_SOURCE_DIR}/..")
set(VCPKG_ROOT "${REPO_ROOT}/third_party/vcpkg" CACHE PATH "vcpkg root")
set(VCPKG_INSTALLED "${VCPKG_ROOT}/installed/x64-mingw-static")

# --- Compiler flags ---
add_compile_options(-Wno-deprecated -Wno-unused-variable -Wno-sign-compare)
add_definitions(-D_WIN32_WINNT=0x0A00 -DWIN32_LEAN_AND_MEAN -DNOMINMAX)
# GIT_COMMIT_HASH is used by layout generators for page titles
add_definitions(-DGIT_COMMIT_HASH="unknown")
# Static linking to avoid DLL dependencies
add_link_options(-static-libgcc -static-libstdc++)

# --- Include directories ---
include_directories(
    ${REPO_ROOT}/src/gen/cpp
    ${REPO_ROOT}/backend/src
    ${REPO_ROOT}/backend/src/database/sqlite
    ${REPO_ROOT}/backend/src/database/debate
    ${REPO_ROOT}/backend/src/utils
    ${REPO_ROOT}/backend/src/debateModerator
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/DebateHandler
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/MoveUserHandler
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/AddClaimHandler
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/ChallengeHandler
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/DeleteClaimHandler
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/EditClaimHandler
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/ConnectClaimsHandler
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/ModifyClaimHandler
    ${REPO_ROOT}/backend/src/debateModerator/buildResponse/homePageResponse
    ${REPO_ROOT}/backend/src/debateModerator/buildResponse/debatePageResponse
    ${REPO_ROOT}/backend/src/debateModerator/buildResponse/loginPageResponse
    ${REPO_ROOT}/backend/src/virtualRenderer
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/homePage
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/loginPage
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/debatePage/SingleStatementView
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/debatePage/FullDebateView
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/debatePage/StepView
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/errorPage
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/historyPage
    ${REPO_ROOT}/backend/src/virtualRenderer/ClientMessageHandler
    ${REPO_ROOT}/backend/src/virtualRenderer/ClientMessageHandler/HomePageEventParser
    ${REPO_ROOT}/backend/src/virtualRenderer/ClientMessageHandler/ErrorPageEventParser
    ${REPO_ROOT}/backend/src/virtualRenderer/ClientMessageHandler/DebatePageEventParser
    ${REPO_ROOT}/backend/src/virtualRenderer/ClientMessageHandler/LoginPageEventParser
    ${REPO_ROOT}/backend/src/virtualRenderer/utils
    ${REPO_ROOT}/backend/src/database/virtualrenderer
    ${VCPKG_INSTALLED}/include
)

# --- Generated protobuf sources ---
file(GLOB PB_SRC "${REPO_ROOT}/src/gen/cpp/*.pb.cc")

# --- Abseil libraries ---
file(GLOB ABSL_LIBS "${VCPKG_INSTALLED}/lib/libabsl_*.a")

# --- Database layer ---
set(DB_SOURCES
    ${REPO_ROOT}/backend/src/database/sqlite/Database.cc
    ${REPO_ROOT}/backend/src/database/sqlite/databaseCommunicator.cc
    ${REPO_ROOT}/backend/src/database/debate/UserDatabase.cc
    ${REPO_ROOT}/backend/src/database/debate/DebateDatabase.cc
    ${REPO_ROOT}/backend/src/database/debate/StatementDatabase.cc
    ${REPO_ROOT}/backend/src/database/debate/DebateMembersDatabase.cc
    ${REPO_ROOT}/backend/src/database/debate/LinkDatabase.cc
    ${REPO_ROOT}/backend/src/database/debate/DatabaseWrapper.cc
    ${REPO_ROOT}/backend/src/utils/DebateWrapper.cc
    ${REPO_ROOT}/backend/src/utils/Log.cc
    ${REPO_ROOT}/backend/src/utils/pathUtils.cpp
)

# --- Event handlers ---
set(HANDLER_SOURCES
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/DebateHandler/DebateHandler.cc
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/MoveUserHandler/MoveUserHandler.cc
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/AddClaimHandler/AddClaimHandler.cc
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/ChallengeHandler/ChallengeHandler.cc
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/DeleteClaimHandler/DeleteClaimHandler.cc
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/EditClaimHandler/EditClaimHandler.cc
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/ConnectClaimsHandler/ConnectClaimsHandler.cc
    ${REPO_ROOT}/backend/src/debateModerator/event-handlers/ModifyClaimHandler/ModifyClaimHandler.cc
)

# --- Moderator ---
set(MODERATOR_SOURCES
    ${REPO_ROOT}/backend/src/debateModerator/DebateModerator.cc
)

# --- Response generators ---
set(RESPONSE_SOURCES
    ${REPO_ROOT}/backend/src/debateModerator/buildResponse/homePageResponse/HomePageResponseGenerator.cc
    ${REPO_ROOT}/backend/src/debateModerator/buildResponse/debatePageResponse/DebatePageResponseGenerator.cc
    ${REPO_ROOT}/backend/src/debateModerator/buildResponse/debatePageResponse/BuildCollection.cc
    ${REPO_ROOT}/backend/src/debateModerator/buildResponse/loginPageResponse/LoginPageResponseGenerator.cc
)

# --- Layout generators ---
set(LAYOUT_SOURCES
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/LayoutGenerator.cc
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/ComponentGenerator.cc
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/homePage/HomePageGenerator.cc
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/homePage/HomePageInfoParser.cc
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/loginPage/LoginPageGenerator.cc
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/debatePage/SingleStatementView/SingleDebatePageGenerator.cc
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/debatePage/SingleStatementView/SingleDebatePageInfoParser.cc
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/debatePage/FullDebateView/FullDebatePageGenerator.cc
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/debatePage/FullDebateView/FullDebatePageInfoParser.cc
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/debatePage/StepView/StepView.cc
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/errorPage/ErrorPageGenerator.cc
    ${REPO_ROOT}/backend/src/virtualRenderer/LayoutGenerator/pages/historyPage/HistoryPageGenerator.cc
)

# --- Additional VR sources needed by layout generators ---
# NOTE: MiddleendRequestHandler.cc, virtualRenderer.cc, parseCookie.cc,
# and ClientMessageParser.cc are excluded because they depend on httplib.h
# (cpp-httplib) which is not available in the test build.
# These are HTTP server components, not needed for event handling tests.
set(VR_EXTRA_SOURCES
    ${REPO_ROOT}/backend/src/virtualRenderer/ClientMessageHandler/LoginPageEventParser/LoginPageEventParser.cc
    ${REPO_ROOT}/backend/src/virtualRenderer/ClientMessageHandler/HomePageEventParser/HomePageEventParser.cc
    ${REPO_ROOT}/backend/src/virtualRenderer/ClientMessageHandler/ErrorPageEventParser/ErrorPageEventParser.cc
    ${REPO_ROOT}/backend/src/virtualRenderer/ClientMessageHandler/DebatePageEventParser/DebatePageEventParser.cc
    ${REPO_ROOT}/backend/src/virtualRenderer/utils/UserNameResolver.cc
    ${REPO_ROOT}/backend/src/database/virtualrenderer/VRUserDatabase.cc
)

# --- Shared link libraries ---
set(COMMON_LIBS
    ${VCPKG_INSTALLED}/lib/libgtest.a
    ${VCPKG_INSTALLED}/lib/manual-link/libgtest_main.a
    ${VCPKG_INSTALLED}/lib/libgmock.a
    ${VCPKG_INSTALLED}/lib/manual-link/libgmock_main.a
    -Wl,--start-group
    ${VCPKG_INSTALLED}/lib/libprotobuf.a
    ${VCPKG_INSTALLED}/lib/libutf8_range.a
    ${VCPKG_INSTALLED}/lib/libutf8_validity.a
    ${ABSL_LIBS}
    dbghelp
    -Wl,--end-group
    ${VCPKG_INSTALLED}/lib/libsqlite3.a
    ${VCPKG_INSTALLED}/lib/libzlib.a
    ws2_32
)

# ============================================================
# ALL TESTS (unit + simulation)
# ============================================================
set(ALL_TEST_SOURCES
    ${CMAKE_SOURCE_DIR}/tests/test_debate_create.cc
    ${CMAKE_SOURCE_DIR}/tests/test_challenge_propagation.cc
    ${CMAKE_SOURCE_DIR}/tests/test_scenario_runner.cc
    ${CMAKE_SOURCE_DIR}/tests/test_step_view_layout.cc
)

add_executable(debate_tests
    ${ALL_TEST_SOURCES}
    ${DB_SOURCES}
    ${HANDLER_SOURCES}
    ${MODERATOR_SOURCES}
    ${RESPONSE_SOURCES}
    ${LAYOUT_SOURCES}
    ${VR_EXTRA_SOURCES}
    ${PB_SRC}
)
target_link_libraries(debate_tests ${COMMON_LIBS})

# Copy scenario .pbtxt files to build dir so tests can find them at runtime
file(GLOB SCENARIO_FILES "${CMAKE_SOURCE_DIR}/scenarios/*.pbtxt")
foreach(scenario_file ${SCENARIO_FILES})
    get_filename_component(scenario_name ${scenario_file} NAME)
    configure_file(${scenario_file} ${CMAKE_BINARY_DIR}/scenarios/${scenario_name} COPYONLY)
endforeach()

# ============================================================
# CTest — register each gtest test individually
# ============================================================
enable_testing()

# Manually register each test so ctest can run them individually
# Format: add_test(NAME Suite.TestName COMMAND exe --gtest_filter=Suite.TestName)

# DebateEventTest (5 tests)
add_test(NAME DebateEventTest.CreateDebate_ViaHandler COMMAND debate_tests --gtest_filter="DebateEventTest.CreateDebate_ViaHandler")
add_test(NAME DebateEventTest.FullEventFlow_CreateDebate_Enter_AddChild COMMAND debate_tests --gtest_filter="DebateEventTest.FullEventFlow_CreateDebate_Enter_AddChild")
add_test(NAME DebateEventTest.AddMultipleChildren_ViaEventPipeline COMMAND debate_tests --gtest_filter="DebateEventTest.AddMultipleChildren_ViaEventPipeline")
add_test(NAME DebateEventTest.NavigateAndAddGrandchild_ViaEventPipeline COMMAND debate_tests --gtest_filter="DebateEventTest.NavigateAndAddGrandchild_ViaEventPipeline")
add_test(NAME DebateEventTest.UserEngagement_StateTransitions COMMAND debate_tests --gtest_filter="DebateEventTest.UserEngagement_StateTransitions")

# PropagationTest (7 tests)
add_test(NAME PropagationTest.ChallengeLeaf_CreatesChallengeClaimAndLink COMMAND debate_tests --gtest_filter="PropagationTest.ChallengeLeaf_CreatesChallengeClaimAndLink")
add_test(NAME PropagationTest.UpdateStatus_ChallengeClaimStatusPropagates COMMAND debate_tests --gtest_filter="PropagationTest.UpdateStatus_ChallengeClaimStatusPropagates")
add_test(NAME PropagationTest.ChallengeRoot_OnlyRootAffected COMMAND debate_tests --gtest_filter="PropagationTest.ChallengeRoot_OnlyRootAffected")
add_test(NAME PropagationTest.MultipleChallenges_OnSameClaim COMMAND debate_tests --gtest_filter="PropagationTest.MultipleChallenges_OnSameClaim")
add_test(NAME PropagationTest.ConcedeChallenge_ClearsUserState COMMAND debate_tests --gtest_filter="PropagationTest.ConcedeChallenge_ClearsUserState")
add_test(NAME PropagationTest.DeleteChallenge_RestoresUndeterminedStatus COMMAND debate_tests --gtest_filter="PropagationTest.DeleteChallenge_RestoresUndeterminedStatus")
add_test(NAME PropagationTest.CreatorSeesOwnClaimAsTrue COMMAND debate_tests --gtest_filter="PropagationTest.CreatorSeesOwnClaimAsTrue")

# ScenarioRunner (8 tests)
add_test(NAME ScenarioRunner.CreateDebate COMMAND debate_tests --gtest_filter="ScenarioRunner.CreateDebate")
add_test(NAME ScenarioRunner.EnterDebate COMMAND debate_tests --gtest_filter="ScenarioRunner.EnterDebate")
add_test(NAME ScenarioRunner.AddChildClaim COMMAND debate_tests --gtest_filter="ScenarioRunner.AddChildClaim")
add_test(NAME ScenarioRunner.ChallengeClaim COMMAND debate_tests --gtest_filter="ScenarioRunner.ChallengeClaim")
add_test(NAME ScenarioRunner.FullStateVerification COMMAND debate_tests --gtest_filter="ScenarioRunner.FullStateVerification")
add_test(NAME ScenarioRunner.CounterChallenge COMMAND debate_tests --gtest_filter="ScenarioRunner.CounterChallenge")
add_test(NAME ScenarioRunner.ConcedeChallenge COMMAND debate_tests --gtest_filter="ScenarioRunner.ConcedeChallenge")
add_test(NAME ScenarioRunner.FullLifecycle COMMAND debate_tests --gtest_filter="ScenarioRunner.FullLifecycle")

# Catch-all
add_test(NAME AllTests COMMAND debate_tests)
