/**
 * getEntirePage.ts
 * 
 * Collects ALL input values from the entire page by scanning the DOM.
 * Returns a complete data object with all field values.
 * 
 * Usage:
 *   const pageData = getEntirePage();
 *   // Returns: { "input-id": "value", "textarea-id": "content", ... }
 */

export interface PageData {
  [key: string]: string | number | boolean | undefined;
  _pageId?: string; // Special field for page ID
}

/**
 * Scans the entire DOM and collects all components (not just inputs).
 * Collects from: input, textarea, select, button, and any element with an ID.
 * Also extracts the page ID from data-page-id attribute.
 * 
 * @param clickedComponentId - Optional ID of the component that was clicked
 * @returns Object mapping element IDs to their current values/states
 */
export function getEntirePage(clickedComponentId?: string): PageData {
  const pageData: PageData = {};
  let fieldCount = 0;

  // Extract page ID from the page-root element
  const pageRoot = document.querySelector('.page-root');
  if (pageRoot) {
    const pageId = pageRoot.getAttribute('data-page-id');
    if (pageId) {
      pageData._pageId = pageId;
      console.log(`📄 Page ID: ${pageId}`);
    }
  }

  // Collect all input fields
  const inputs = document.querySelectorAll<HTMLInputElement>('input[id]');
  inputs.forEach((input) => {
    if (!input.id) return;
    
    fieldCount++;
    if (input.type === 'checkbox' || input.type === 'radio') {
      pageData[input.id] = input.checked;
    } else if (input.type === 'number') {
      const numValue = parseFloat(input.value);
      pageData[input.id] = isNaN(numValue) ? input.value : numValue;
    } else {
      pageData[input.id] = input.value || '';
    }
  });

  // Collect all textarea fields
  const textareas = document.querySelectorAll<HTMLTextAreaElement>('textarea[id]');
  textareas.forEach((textarea) => {
    if (!textarea.id) return;
    
    fieldCount++;
    pageData[textarea.id] = textarea.value || '';
  });

  // Collect all select fields
  const selects = document.querySelectorAll<HTMLSelectElement>('select[id]');
  selects.forEach((select) => {
    if (!select.id) return;
    
    fieldCount++;
    if (select.multiple) {
      // For multi-select, join selected values
      const selectedValues = Array.from(select.selectedOptions).map(opt => opt.value);
      pageData[select.id] = selectedValues.join(',');
    } else {
      pageData[select.id] = select.value || '';
    }
  });

  // Collect all buttons with clicked state
  const buttons = document.querySelectorAll<HTMLButtonElement>('button[id]');
  buttons.forEach((button) => {
    if (!button.id) return;
    
    fieldCount++;
    // Mark if this button was clicked
    pageData[button.id] = button.id === clickedComponentId ? 'clicked' : 'not-clicked';
  });

  // Collect all other elements with IDs (divs, spans, etc.)
  const allElements = document.querySelectorAll<HTMLElement>('[id]');
  allElements.forEach((element) => {
    if (!element.id) return;
    
    // Skip if already collected (input, textarea, select, button)
    if (pageData.hasOwnProperty(element.id)) return;
    
    fieldCount++;
    
    // Get text content or empty string
    const textContent = element.textContent?.trim() || '';
    pageData[element.id] = textContent;
  });

  console.log(`📦 getEntirePage collected ${fieldCount} components:`, {
    inputCount: inputs.length,
    textareaCount: textareas.length,
    selectCount: selects.length,
    buttonCount: buttons.length,
    otherElements: allElements.length - inputs.length - textareas.length - selects.length - buttons.length,
    totalFields: Object.keys(pageData).length,
  });

  return pageData;
}

/**
 * Get a specific input value by ID.
 * 
 * @param inputId - The ID of the element to retrieve
 * @returns The element's current value, or null if not found
 */
export function getInputValue(inputId: string): string | number | boolean | null {
  const element = document.getElementById(inputId);
  
  if (!element) {
    console.warn(`⚠️ Element with id "${inputId}" not found`);
    return null;
  }

  if (element instanceof HTMLInputElement) {
    if (element.type === 'checkbox' || element.type === 'radio') {
      return element.checked;
    } else if (element.type === 'number') {
      const numValue = parseFloat(element.value);
      return isNaN(numValue) ? element.value : numValue;
    }
    return element.value;
  }

  if (element instanceof HTMLTextAreaElement || element instanceof HTMLSelectElement) {
    return element.value;
  }

  console.warn(`⚠️ Element "${inputId}" is not an input/textarea/select`);
  return null;
}

/**
 * Get all input IDs currently on the page.
 * Useful for debugging.
 * 
 * @returns Array of all input element IDs found on the page
 */
export function getAllInputIds(): string[] {
  const ids: string[] = [];
  
  document.querySelectorAll<HTMLInputElement>('input[id]').forEach(el => {
    if (el.id) ids.push(el.id);
  });
  
  document.querySelectorAll<HTMLTextAreaElement>('textarea[id]').forEach(el => {
    if (el.id) ids.push(el.id);
  });
  
  document.querySelectorAll<HTMLSelectElement>('select[id]').forEach(el => {
    if (el.id) ids.push(el.id);
  });
  
  return ids;
}

/**
 * Check if page has any input fields.
 * 
 * @returns True if page has at least one input/textarea/select with an ID
 */
export function hasInputFields(): boolean {
  return (
    document.querySelectorAll('input[id], textarea[id], select[id]').length > 0
  );
}

// Export default function for convenience
export default getEntirePage;
