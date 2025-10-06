import { runAllTests, testGetComponentInfo, testLiveInputMonitoring } from "./test";
import { testGetAllInputValues } from "./test";

// Test a specific component
testGetComponentInfo("joinRoomForm");

// Test all inputs on page
testGetAllInputValues();

// Monitor live typing
const stopMonitor = testLiveInputMonitoring("joinRoomForm");
// Type in the input field and watch console
// stopMonitor(); // when done

// Run all tests
runAllTests();