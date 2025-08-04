import React from "react";
import ComponentRenderer from "./ComponentRenderer.jsx";

export function PageRenderer({ page }) {
  if (!page?.components) return null;

  return (
    <div className="page-root">
      {page.components.map((component, idx) => (
        <ComponentRenderer key={component.id || idx} component={component} />
      ))}
    </div>
  );
}
