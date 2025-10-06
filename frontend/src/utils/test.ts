import getInfoFromPage, { getAllCurrentInputValues } from './getInfoFromPage.js';

/**
 * Test the getInfoFromPage functionality
 * Run this from the browser console or call it from your app
 */

// Test 1: Test a specific component by ID
export function testGetComponentInfo(componentId: string = "joinRoomForm") {
  console.log("🧪 Test 1: Getting component info for:", componentId);
  
  const result = getInfoFromPage(componentId);
  
  if (result !== null) {
    console.log(`✅ Success! Component '${componentId}' value:`, result);
  } else {
    console.log("❌ Component not found or has no value!");
  }
  
  return result;
}

// Test 2: Test multiple components at once
export function testMultipleComponents() {
  console.log("🧪 Test 2: Testing multiple components...");
  
  // Common component IDs that might exist on your pages
  const testIds = [
    "joinRoomForm", 
    "searchInput", 
    "usernameInput", 
    "btnCreateRoom",
    "roomNameInput",
    "passwordInput"
  ];
  
  const results: Record<string, any> = {};
  
  for (const id of testIds) {
    const result = getInfoFromPage(id);
    results[id] = result;
    
    if (result !== null) {
      console.log(`✅ Found: ${id} - Current value: "${result}"`);
    } else {
      console.log(`❌ Not found: ${id}`);
    }
  }
  
  console.log("📊 All results:", results);
  return results;
}

// Test 3: Test live input monitoring
export function testLiveInputMonitoring(componentId: string = "joinRoomForm") {
  console.log(`🧪 Test 3: Monitoring live input for: ${componentId}`);
  console.log("Type something in the input and watch the console...");
  
  let lastValue = "";
  
  const monitor = setInterval(() => {
    const currentValue = getInfoFromPage(componentId);
    
    if (currentValue !== null && currentValue !== lastValue) {
      console.log(`🔄 Input changed: "${lastValue}" → "${currentValue}"`);
      lastValue = currentValue;
    }
  }, 500); // Check every 500ms
  
  // Return a function to stop monitoring
  return () => {
    clearInterval(monitor);
    console.log(`⏹️ Stopped monitoring ${componentId}`);
  };
}

// Test 4: Get all input values at once
export function testGetAllInputValues() {
  console.log("🧪 Test 4: Getting all input values...");
  
  const allValues = getAllCurrentInputValues();
  
  console.log("📋 All current input values:");
  console.table(allValues);
  
  const count = Object.keys(allValues).length;
  console.log(`✅ Found ${count} input elements with values`);
  
  return allValues;
}

// Test 5: Interactive test - prompts user to type
export function testInteractive() {
  console.log("🧪 Test 5: Interactive test");
  console.log("1. Find an input field on the page");
  console.log("2. Type something in it");
  console.log("3. Run: testGetComponentInfo('yourInputId')");
  console.log("4. See if it captures what you typed!");
  
  // Show all available inputs
  const allInputs = getAllCurrentInputValues();
  const inputIds = Object.keys(allInputs);
  
  if (inputIds.length > 0) {
    console.log("🎯 Available input IDs to test with:");
    inputIds.forEach(id => console.log(`   - ${id}`));
    
    // Test the first one found
    const firstId = inputIds[0];
    console.log(`\n🔬 Testing first input: ${firstId}`);
    testGetComponentInfo(firstId);
  } else {
    console.log("❌ No input elements found on this page");
  }
}

// Run all tests
export function runAllTests() {
  console.log("🚀 Running all getInfoFromPage tests...\n");
  
  testInteractive();
  console.log("\n" + "=".repeat(50) + "\n");
  
  testMultipleComponents();
  console.log("\n" + "=".repeat(50) + "\n");
  
  testGetAllInputValues();
  console.log("\n" + "=".repeat(50) + "\n");
  
  console.log("✅ All tests completed!");
  console.log("💡 Try: testLiveInputMonitoring('someInputId') for real-time monitoring");
}

// Make functions available globally for easy testing
if (typeof window !== 'undefined') {
  (window as any).testGetComponentInfo = testGetComponentInfo;
  (window as any).testMultipleComponents = testMultipleComponents;
  (window as any).testLiveInputMonitoring = testLiveInputMonitoring;
  (window as any).testGetAllInputValues = testGetAllInputValues;
  (window as any).testInteractive = testInteractive;
  (window as any).runAllTests = runAllTests;
  (window as any).getInfoFromPage = getInfoFromPage;
  
  console.log("🎮 Test functions available globally:");
  console.log("- testGetComponentInfo('componentId')");
  console.log("- testMultipleComponents()");
  console.log("- testLiveInputMonitoring('componentId')");
  console.log("- testGetAllInputValues()");
  console.log("- testInteractive()");
  console.log("- runAllTests()");
  console.log("- getInfoFromPage('componentId')");
}