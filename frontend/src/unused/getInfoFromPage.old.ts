// import getPageFromBackend from "../backendCommunicator/getPageFromBackend.js";

// // TypeScript interfaces for component data
// interface Component {
//   id?: string;
//   type?: string;
//   text?: string;
//   value?: string;
//   placeholder?: string;
//   children?: Component[];
//   style?: any;
//   css?: Record<string, string>;
//   events?: Record<string, string>;
//   [key: string]: any;
// }

// interface PageData {
//   components?: Component[];
//   [key: string]: any;
// }

// interface ComponentInfo {
//   id: string;
//   type: string;
//   text?: string;
//   value?: string;
//   placeholder?: string;
//   style?: any;
//   css?: Record<string, string>;
//   events?: Record<string, string>;
//   path: string; // Path to the component (e.g., "components[0].children[1]")
//   found: boolean;
// }

// /**
//  * Recursively search for a component by ID in the component tree
//  * @param components - Array of components to search through
//  * @param componentId - The ID of the component to find
//  * @param basePath - Current path in the component tree
//  * @returns ComponentInfo object with component data and path, or null if not found
//  */
// function findComponentById(components: Component[], componentId: string, basePath: string = "components"): ComponentInfo | null {
//   for (let i = 0; i < components.length; i++) {
//     const component = components[i];
//     const currentPath = `${basePath}[${i}]`;
    
//     // Check if this is the component we're looking for
//     if (component.id === componentId) {
//       return {
//         id: component.id,
//         type: component.type || "unknown",
//         text: component.text,
//         value: component.value,
//         placeholder: component.placeholder,
//         style: component.style,
//         css: component.css,
//         events: component.events,
//         path: currentPath,
//         found: true
//       };
//     }
    
//     // If this component has children, search recursively
//     if (component.children && component.children.length > 0) {
//       const childResult = findComponentById(component.children, componentId, `${currentPath}.children`);
//       if (childResult) {
//         return childResult;
//       }
//     }
//   }
  
//   return null;
// }

// /**
//  * Get information about a specific component by its ID
//  * @param componentId - The ID of the component to find
//  * @param endpoint - Optional API endpoint to fetch page data from (defaults to /api/data)
//  * @returns Promise<ComponentInfo | null> - Component information or null if not found
//  */
// export async function getComponentInfo(componentId: string, endpoint?: string): Promise<ComponentInfo | null> {
//   try {
//     // Fetch the current page data
//     const pageData: PageData | null = await getPageFromBackend(endpoint);
    
//     if (!pageData || !pageData.components) {
//       console.warn("getComponentInfo: No page data or components found");
//       return null;
//     }
    
//     // Search for the component
//     const componentInfo = findComponentById(pageData.components, componentId);
    
//     if (componentInfo) {
//       console.log(`✅ Found component '${componentId}':`, componentInfo);
//       return componentInfo;
//     } else {
//       console.warn(`⚠️ Component '${componentId}' not found in page`);
//       return {
//         id: componentId,
//         type: "not-found",
//         path: "",
//         found: false
//       };
//     }
    
//   } catch (error) {
//     console.error("Error getting component info:", error);
//     return null;
//   }
// }

// /**
//  * Get information about a text box component specifically
//  * @param componentId - The ID of the text box component
//  * @param endpoint - Optional API endpoint to fetch page data from
//  * @returns Promise<ComponentInfo | null> - Text box information or null if not found or not a text box
//  */
// export async function getTextBoxInfo(componentId: string, endpoint?: string): Promise<ComponentInfo | null> {
//   const componentInfo = await getComponentInfo(componentId, endpoint);
  
//   if (!componentInfo || !componentInfo.found) {
//     return componentInfo;
//   }
  
//   // Check if it's actually a text box component
//   const isTextBox = componentInfo.type === "INPUT" || 
//                    componentInfo.type === "TEXTAREA" ||
//                    componentInfo.type === "TEXT_INPUT";
  
//   if (!isTextBox) {
//     console.warn(`⚠️ Component '${componentId}' is not a text box (type: ${componentInfo.type})`);
//     return {
//       ...componentInfo,
//       found: false
//     };
//   }
  
//   console.log(`✅ Found text box '${componentId}':`, {
//     text: componentInfo.text,
//     value: componentInfo.value,
//     placeholder: componentInfo.placeholder,
//     type: componentInfo.type
//   });
  
//   return componentInfo;
// }

// export default getComponentInfo;