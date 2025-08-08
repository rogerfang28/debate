import React from "react";
import ComponentRenderer from "./ComponentRenderer";

// TypeScript interfaces
interface Component {
  id?: string;
  [key: string]: any;
}

interface Page {
  components?: Component[];
  [key: string]: any;
}

interface PageRendererProps {
  page: Page;
}

export const PageRenderer: React.FC<PageRendererProps> = ({ page }) => {
  if (!page?.components) return null;

  return (
    <div className="page-root">
      {page.components.map((component: Component, idx: number) => (
        <ComponentRenderer key={component.id || idx} component={component} />
      ))}
    </div>
  );
};
