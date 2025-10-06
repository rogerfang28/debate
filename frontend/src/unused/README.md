# Component Info Extraction System

This system allows you to find specific components on the current page and extract their information, such as text content, input values, styling, and more.

## Files Overview

- **`getInfoFromPage.ts`** - Main module with component search and extraction functions
- **`exampleUsage.ts`** - Examples showing how to use the system
- **`getPageFromBackend.ts`** - Fetches page data from the backend API

## Quick Start

### 1. Get Component Info by ID

```typescript
import { getComponentInfo } from "./getInfoFromPage.js";

// Find any component by its ID
const componentInfo = await getComponentInfo("joinRoomForm");

if (componentInfo && componentInfo.found) {
  console.log("Found component:", {
    id: componentInfo.id,
    type: componentInfo.type,
    value: componentInfo.value,
    text: componentInfo.text,
    placeholder: componentInfo.placeholder,
    path: componentInfo.path
  });
}
```

### 2. Get Text Box Info Specifically

```typescript
import { getTextBoxInfo } from "./getInfoFromPage.js";

// Find text input/textarea components only
const textBoxInfo = await getTextBoxInfo("joinRoomForm");

if (textBoxInfo && textBoxInfo.found) {
  console.log("Text box value:", textBoxInfo.value || textBoxInfo.text);
  console.log("Placeholder:", textBoxInfo.placeholder);
}
```

## Available Functions

### `getComponentInfo(componentId, endpoint?)`
- **Purpose**: Find any component by ID and extract its information
- **Parameters**: 
  - `componentId` (string) - The ID of the component to find
  - `endpoint` (string, optional) - API endpoint to fetch page from (defaults to `/api/data`)
- **Returns**: `Promise<ComponentInfo | null>`

### `getTextBoxInfo(componentId, endpoint?)`
- **Purpose**: Find text input components specifically (INPUT/TEXTAREA types)
- **Parameters**: Same as `getComponentInfo`
- **Returns**: `Promise<ComponentInfo | null>` - Only returns if component is a text input

## ComponentInfo Structure

```typescript
interface ComponentInfo {
  id: string;              // Component ID
  type: string;            // Component type (INPUT, BUTTON, TEXT, etc.)
  text?: string;           // Text content
  value?: string;          // Input value (for form fields)
  placeholder?: string;    // Placeholder text
  style?: any;             // Styling information
  css?: Record<string, string>; // CSS properties
  events?: Record<string, string>; // Event handlers
  path: string;            // Location in component tree
  found: boolean;          // Whether component was found
}
```

## Real Component IDs You Can Test With

Based on the current page schemas, here are some actual component IDs you can use:

### Home Page Components
- `"joinRoomForm"` - Input field for joining rooms
- `"btnCreateRoom"` - Create room button
- `"btnPublicDebates"` - Public debates button
- `"btnFriends"` - Friends button
- `"btnProfile"` - Profile button

### Other Pages
- `"searchInput"` - Search field (Public Debates page)
- `"usernameInput"` - Username field (Profile page)

## Usage Examples

### Check Form Field Values
```typescript
async function checkFormFields() {
  const joinRoomValue = await getTextBoxInfo("joinRoomForm");
  
  if (joinRoomValue && joinRoomValue.found) {
    console.log("User entered:", joinRoomValue.value);
    
    // You can now use this value for validation or processing
    if (!joinRoomValue.value || joinRoomValue.value.trim() === "") {
      console.log("Field is empty!");
    }
  }
}
```

### Monitor Component Changes
```typescript
import { exampleMonitorComponent } from "./exampleUsage.js";

// Monitor a component for changes every 500ms
const stopMonitoring = exampleMonitorComponent("joinRoomForm", 500);

// Stop monitoring later
// stopMonitoring();
```

### Find All Input Components
```typescript
import { exampleFindAllInputs } from "./exampleUsage.js";

// Get a list of all input components on the current page
const inputs = await exampleFindAllInputs();
console.log("All input components:", inputs);
```

## How It Works

1. **Page Fetching**: Uses `getPageFromBackend()` to fetch the current page as protobuf data
2. **Tree Traversal**: Recursively searches through the component hierarchy 
3. **Component Matching**: Finds components by matching their `id` property
4. **Data Extraction**: Extracts relevant information (text, value, styling, etc.)
5. **Path Tracking**: Keeps track of the component's location in the tree

## Error Handling

The system includes comprehensive error handling:
- Returns `null` if page data is unavailable
- Returns `{found: false}` if component is not found
- Logs warnings for missing components
- Gracefully handles network errors and parsing issues

## Integration with Event System

This component info extraction system works perfectly with the existing event system:

```typescript
// Extract form data before sending an event
async function submitForm() {
  const roomName = await getTextBoxInfo("joinRoomForm");
  
  if (roomName && roomName.found && roomName.value) {
    // Send the extracted value via the event system
    await sendData("joinRoom", { roomCode: roomName.value });
  }
}
```

This provides a powerful way to get real-time information about components on the current page, perfect for form validation, data extraction, and dynamic UI interactions!
