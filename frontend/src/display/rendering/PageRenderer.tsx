import React from "react";
import ComponentRenderer from "./ComponentRenderer";
import type { Page, Component } from "../../../../src/gen/ts/layout_pb.ts";

interface PageRendererProps {
  page: Page;
}

export const PageRenderer: React.FC<PageRendererProps> = ({ page }) => {
  if (!page?.components || page.components.length === 0) return null;

  return (
    <div className="page-root" data-page-id={page.pageId}>
      {page.title && (
        <div className="page-title" role="heading" aria-level={1}>
          {page.title}
        </div>
      )}
      {page.components.map((component: Component, idx: number) => (
        <ComponentRenderer key={component.id || idx} component={component} />
      ))}
    </div>
  );
};
