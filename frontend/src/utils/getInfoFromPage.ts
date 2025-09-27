/**
 * Get value from any input element by ID
 * @param componentId - The ID of the component
 * @returns Current value or null if not found
 */
export function getInfoFromPage(componentId: string): string | null {
  const element = document.getElementById(componentId) as HTMLInputElement;
  return element?.value || null;
}

/**
 * Get current values from all input elements on the page
 * @returns Object with all input values keyed by their IDs
 */
export function getAllCurrentInputValues(): Record<string, string> {
  const inputValues: Record<string, string> = {};
  
  // Get all input elements
  const inputs = document.querySelectorAll('input, textarea, select');
  
  inputs.forEach(element => {
    if (element.id) {
      if (element instanceof HTMLInputElement || 
          element instanceof HTMLTextAreaElement || 
          element instanceof HTMLSelectElement) {
        inputValues[element.id] = element.value;
      }
    }
  });
  
  return inputValues;
}

/**
 * Get live value from a specific input component
 * @param componentId - The ID of the input component
 * @returns Current value or null if not found/not an input
 */
export function getCurrentInputValue(componentId: string): string | null {
  const element = document.getElementById(componentId);
  
  if (element instanceof HTMLInputElement || 
      element instanceof HTMLTextAreaElement || 
      element instanceof HTMLSelectElement) {
    return element.value;
  }
  
  return null;
}

export default getInfoFromPage;