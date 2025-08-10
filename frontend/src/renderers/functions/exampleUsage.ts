import { getComponentInfo, getTextBoxInfo } from "./getInfoFromPage.js";
import getPageFromBackend from "./getPageFromBackend.js";

/**
 * Example usage of the component info extraction functions
 * This shows how to find and extract information from specific components
 */

// Example 1: Get info from any component by ID
export async function exampleGetComponentInfo() {
  console.log("🔍 Example: Getting component info...");
  
  // Find a component with ID "joinRoomForm" (from the home page)
  const componentInfo = await getComponentInfo("joinRoomForm");
  
  if (componentInfo && componentInfo.found) {
    console.log("✅ Component found:", {
      id: componentInfo.id,
      type: componentInfo.type,
      text: componentInfo.text,
      value: componentInfo.value,
      placeholder: componentInfo.placeholder,
      path: componentInfo.path
    });
    
    // You can use this info to:
    // - Check if a form field has been filled out
    // - Get the current value of an input
    // - Validate form data
    // - Extract component styling information
  } else {
    console.log("❌ Component not found or error occurred");
  }
}

// Example 2: Get info specifically from text box components
export async function exampleGetTextBoxInfo() {
  console.log("🔍 Example: Getting text box info...");
  
  // Look for a text input component from the home page
  const textBoxInfo = await getTextBoxInfo("joinRoomForm");
  
  if (textBoxInfo && textBoxInfo.found) {
    console.log("✅ Text box found:", {
      currentValue: textBoxInfo.value || textBoxInfo.text,
      placeholder: textBoxInfo.placeholder,
      inputType: textBoxInfo.type
    });
    
    // You can use this to:
    // - Extract user input from forms
    // - Validate text input before submission
    // - Check if required fields are filled
  } else {
    console.log("❌ Text box not found or component is not a text input");
  }
}

// Example 3: Check multiple real components at once
export async function exampleCheckFormFields() {
  console.log("🔍 Example: Checking multiple form fields...");
  
  // These are real component IDs from different pages
  const fieldIds = ["joinRoomForm", "searchInput", "usernameInput", "btnCreateRoom"];
  const formData: Record<string, any> = {};
  
  for (const fieldId of fieldIds) {
    const info = await getComponentInfo(fieldId);
    if (info && info.found) {
      formData[fieldId] = {
        type: info.type,
        value: info.value || info.text,
        placeholder: info.placeholder,
        path: info.path
      };
    } else {
      formData[fieldId] = { found: false };
    }
  }
  
  console.log("📋 Form data extracted:", formData);
  
  // You can use this to:
  // - Build a complete form submission object
  // - Validate all fields before sending data
  // - Create dynamic form processing
}

// Example 4: Real-time component monitoring
export async function exampleMonitorComponent(componentId: string = "joinRoomForm", intervalMs: number = 1000) {
  console.log(`🔄 Example: Monitoring component '${componentId}' every ${intervalMs}ms...`);
  
  let lastValue: string | undefined = "";
  
  const monitor = setInterval(async () => {
    const info = await getComponentInfo(componentId);
    
    if (info && info.found) {
      const currentValue = info.value || info.text || "";
      
      if (currentValue !== lastValue) {
        console.log(`🔄 Component '${componentId}' changed:`, {
          from: lastValue,
          to: currentValue,
          timestamp: new Date().toISOString()
        });
        lastValue = currentValue;
      }
    }
  }, intervalMs);
  
  // Return a function to stop monitoring
  return () => {
    clearInterval(monitor);
    console.log(`⏹️ Stopped monitoring component '${componentId}'`);
  };
}

// Example 5: Find all input components on the current page
export async function exampleFindAllInputs() {
  console.log("🔍 Example: Finding all input components...");
  
  // Get the full page data first
  const pageData = await getPageFromBackend();
  
  if (!pageData || !pageData.components) {
    console.log("❌ No page data available");
    return;
  }
  
  const inputComponents: any[] = [];
  
  function searchForInputs(components: any[], path: string = "components") {
    components.forEach((comp, index) => {
      if (comp.type === "INPUT" || comp.type === "TEXTAREA") {
        inputComponents.push({
          id: comp.id,
          type: comp.type,
          text: comp.text,
          value: comp.value,
          placeholder: comp.placeholder,
          path: `${path}[${index}]`
        });
      }
      
      if (comp.children && comp.children.length > 0) {
        searchForInputs(comp.children, `${path}[${index}].children`);
      }
    });
  }
  
  searchForInputs(pageData.components);
  
  console.log(`✅ Found ${inputComponents.length} input components:`, inputComponents);
  return inputComponents;
}

// You can call these examples like:
// exampleGetComponentInfo();
// exampleGetTextBoxInfo();
// exampleCheckFormFields();
// exampleFindAllInputs();
// const stopMonitoring = exampleMonitorComponent("joinRoomForm", 500);
