// frontend/src/functions/renderPage.jsx
import React from "react";
import { ComponentType } from "../../../../protos/page_pb.js";
import sendDataToBackend from "./sendData.js";
import { EventType } from "../../../../protos/event_pb.js"; // adjust path

// Constants
const CSS_CLASSES = {
  PAGE_ROOT: 'page-root',
  COMPONENT_ERROR: 'component-error',
  PAGE_ERROR: 'page-root error',
  LIST_ITEM: 'list-item',
  CARD_BASE: 'card p-4 shadow rounded',
  ICON: 'icon',
};

const UNKNOWN_COMPONENT_ID = 'unknown';

/**
 * Frontend-only event handlers for special cases
 * @type {Object<string, Function>}
 */
const eventHandlers = {
  goProfile: () => alert("Go to profile"),
  startExperience: () => alert("Starting experience..."),
  sayHello: () => alert("Hello from backend event!"),
};

/**
 * Maps string event names from backend to EventType enum values
 * @type {Object<string, number>}
 */
const eventTypeMap = {
  onClick: EventType.CLICK,
  onChange: EventType.INPUT_CHANGE,
  onSubmit: EventType.FORM_SUBMIT,
};

/**
 * Builds CSS class names from component style properties
 * @param {Object} style - The style object from component
 * @returns {string} Combined class names
 */
function buildClassName(style) {
  if (!style || typeof style !== 'object') return "";
  
  return [
    style.customClass || "",
    style.padding || "",
    style.margin || "",
    style.bgColor || style.bg_color || "",
    style.textColor || style.text_color || "",
    style.border || "",
    style.rounded || "",
    style.shadow || "",
    style.gap || "",
  ]
    .filter(Boolean)
    .join(" ");
}

/**
 * Generic event handler that sends all event details to backend and triggers page reload
 * @param {Event} e - The DOM event
 * @param {Object} component - The component object
 * @param {string} eventName - The original event name (onClick, onChange, etc.)
 * @param {string} handlerName - The handler name from component config
 * @param {Function} onReload - Callback function to trigger page reload
 */
function handleEvent(e, component, eventName, handlerName, onReload) {
  try {
    // Prevent default behavior for form events
    if (e?.preventDefault && (eventName === 'onSubmit' || e.type === 'submit')) {
      e.preventDefault();
    }

    // Collect comprehensive event data
    const eventData = {
      componentId: component.id || UNKNOWN_COMPONENT_ID,
      eventType: eventTypeMap[eventName] || EventType.UNKNOWN,
      handlerName,
      eventName,
      data: {
        value: e?.target?.value,
        checked: e?.target?.checked,
        type: e?.target?.type,
        name: e?.target?.name,
        text: component.text,
        tagName: e?.target?.tagName,
        className: e?.target?.className,
      },
      timestamp: Date.now(),
    };

    // Send to backend
    sendDataToBackend(eventData);

    // Call local handler if it exists (optional)
    if (eventHandlers[handlerName]) {
      eventHandlers[handlerName](e);
    }

    // Trigger page reload after a short delay to allow backend processing
    setTimeout(() => {
      if (onReload) onReload();
    }, 100);

  } catch (error) {
    console.error(`Error handling event ${eventName}:`, error);
  }
}

/**
 * Creates event handlers for component events using the generic handleEvent function
 * @param {Object} events - Event configuration from component
 * @param {Object} component - The component object
 * @param {Function} onReload - Callback function to trigger page reload
 * @returns {Object} React event handlers
 */
function createEventHandlers(events, component, onReload) {
  const boundEvents = {};
  
  if (!events || typeof events !== 'object') return boundEvents;

  try {
    for (const [eventName, handlerName] of Object.entries(events)) {
      if (typeof eventName !== 'string' || typeof handlerName !== 'string') {
        console.warn(`Invalid event configuration: ${eventName} -> ${handlerName}`);
        continue;
      }

      const reactEventName = eventName.charAt(0).toLowerCase() + eventName.slice(1);
      boundEvents[reactEventName] = (e) => handleEvent(e, component, eventName, handlerName, onReload);
    }
  } catch (error) {
    console.error('Error creating event handlers:', error);
  }

  return boundEvents;
}

/**
 * Renders child components recursively with error handling
 * @param {Array} children - Array of child components
 * @param {Function} onReload - Function to trigger page reload
 * @returns {JSX.Element[]|null} Rendered child components or null if no valid children
 */
function renderChildren(children, onReload) {
  if (!children || !Array.isArray(children) || children.length === 0) {
    return null;
  }
  
  try {
    return children
      .filter(child => child && typeof child === 'object') // Filter out invalid children
      .map((child, idx) => {
        // Ensure each child has a unique key
        const key = child.id || `child-${idx}`;
        return <ComponentRenderer key={key} component={child} onReload={onReload} />;
      });
  } catch (error) {
    console.error('Error rendering children:', error);
    return null;
  }
}

/**
 * Component renderer factories for different component types
 */
const componentRenderers = {
  [ComponentType.TEXT]: ({ text, className, inlineStyle, boundEvents }) => (
    <p className={className} style={inlineStyle} {...boundEvents}>
      {text}
    </p>
  ),

  [ComponentType.BUTTON]: ({ text, className, inlineStyle, boundEvents }) => (
    <button className={className} style={inlineStyle} {...boundEvents}>
      {text}
    </button>
  ),

  [ComponentType.IMAGE]: ({ component, text, className, inlineStyle, boundEvents }) => (
    <img 
      src={component.value} 
      alt={text || ""} 
      className={className} 
      style={inlineStyle} 
      {...boundEvents} 
    />
  ),

  [ComponentType.LIST]: ({ items, className, inlineStyle, boundEvents }) => (
    <ul className={className} style={inlineStyle} {...boundEvents}>
      {items?.map((item, idx) => (
        <li key={idx} className={CSS_CLASSES.LIST_ITEM}>
          {item.icon && <span className={`${CSS_CLASSES.ICON} ${item.icon}`} />} {item.label}
        </li>
      ))}
    </ul>
  ),

  [ComponentType.CONTAINER]: ({ className, inlineStyle, boundEvents, children, onReload }) => (
    <div className={className} style={inlineStyle} {...boundEvents}>
      {renderChildren(children, onReload)}
    </div>
  ),

  [ComponentType.CARD]: ({ className, inlineStyle, boundEvents, children, onReload }) => (
    <div className={`${CSS_CLASSES.CARD_BASE} ${className}`} style={inlineStyle} {...boundEvents}>
      {renderChildren(children, onReload)}
    </div>
  ),
};

/**
 * Default renderer for unknown component types
 * @param {Object} props - Renderer properties
 * @param {string} props.className - CSS class names
 * @param {Object} props.inlineStyle - Inline CSS styles
 * @param {Object} props.boundEvents - Event handlers
 * @param {Array} props.children - Child components
 * @param {Function} props.onReload - Function to trigger page reload
 * @returns {JSX.Element} Default div container
 */
const defaultRenderer = ({ className, inlineStyle, boundEvents, children, onReload }) => (
  <div className={className} style={inlineStyle} {...boundEvents}>
    {renderChildren(children, onReload)}
  </div>
);

/**
 * React component that renders a complete page from a page component definition with auto-reload on events
 * @param {Object} props - Component props
 * @param {Object} props.page - The page object containing components
 * @param {Array} props.page.components - Array of component definitions
 * @returns {JSX.Element|null} The rendered page or null if no components
 * @example
 * const page = {
 *   components: [
 *     { type: ComponentType.TEXT, text: "Hello World" },
 *     { type: ComponentType.BUTTON, text: "Click me", events: { onClick: "handleClick" } }
 *   ]
 * };
 * return <PageRenderer page={page} />;
 */
export function PageRenderer({ page }) {
  const [reloadKey, setReloadKey] = React.useState(0);

  // Validate input
  if (!page || typeof page !== 'object') {
    console.warn('PageRenderer: Invalid page object provided');
    return null;
  }
  
  if (!page.components || !Array.isArray(page.components)) {
    console.warn('PageRenderer: No valid components array found');
    return null;
  }

  // Reload function to trigger re-render
  const triggerReload = React.useCallback(() => {
    setReloadKey(prev => prev + 1);
  }, []);

  try {
    return (
      <div key={reloadKey} className={CSS_CLASSES.PAGE_ROOT}>
        {page.components.map((component, idx) => (
          <ComponentRenderer 
            key={`${component?.id || idx}-${reloadKey}`} 
            component={component} 
            onReload={triggerReload}
          />
        ))}
      </div>
    );
  } catch (error) {
    console.error('Error rendering page:', error);
    return (
      <div className={CSS_CLASSES.PAGE_ERROR}>
        <p>Error rendering page content</p>
      </div>
    );
  }
}

/**
 * Legacy function wrapper for backwards compatibility
 * @param {Object} page - The page object containing components
 * @returns {JSX.Element|null} The rendered page component
 * @deprecated Use PageRenderer component instead
 */
export default function renderPage(page) {
  return <PageRenderer page={page} />;
}

/**
 * Renders an individual component based on its type and properties
 * @param {Object} props - Component renderer props
 * @param {Object} props.component - The component definition object
 * @param {Function} props.onReload - Function to trigger page reload
 * @param {string} props.component.type - Component type from ComponentType enum
 * @param {string} [props.component.text] - Text content for the component
 * @param {Array} [props.component.children] - Child components
 * @param {Object} [props.component.style] - Style configuration object
 * @param {Object} [props.component.css] - Inline CSS styles
 * @param {Object} [props.component.events] - Event handler configuration
 * @param {Array} [props.component.items] - Items for list components
 * @returns {JSX.Element|null} The rendered component or null if component is invalid
 * @example
 * <ComponentRenderer 
 *   component={{
 *     type: ComponentType.BUTTON,
 *     text: "Click me",
 *     events: { onClick: "handleClick" }
 *   }}
 *   onReload={() => console.log('Reloading...')}
 * />
 */
function ComponentRenderer({ component, onReload }) {
  // Validate component input
  if (!component || typeof component !== 'object') {
    console.warn('ComponentRenderer: Invalid component provided');
    return null;
  }

  if (component.type === undefined || component.type === null) {
    console.warn('ComponentRenderer: Component missing required type property');
    return null;
  }

  try {
    const { type, text, children, style, css, events, items } = component;

    const className = buildClassName(style);
    const inlineStyle = css || {};
    const boundEvents = createEventHandlers(events, component, onReload);

    // Get the appropriate renderer or fall back to default
    const renderer = componentRenderers[type] || defaultRenderer;
    
    // Prepare props for the renderer
    const rendererProps = {
      component,
      text,
      children,
      items,
      className,
      inlineStyle,
      boundEvents,
      onReload, // Pass onReload for nested components
    };

    return renderer(rendererProps);
  } catch (error) {
    console.error('Error rendering component:', error, component);
    return (
      <div className={CSS_CLASSES.COMPONENT_ERROR}>
        <span>Error rendering component</span>
      </div>
    );
  }
}
