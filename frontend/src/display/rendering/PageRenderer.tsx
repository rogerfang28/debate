import React from "react";
import ComponentRenderer from "./ComponentRenderer";
import type { Page, Component } from "../../../../src/gen/ts/layout_pb.ts";

interface PageRendererProps {
  page: Page;
}

export const PageRenderer: React.FC<PageRendererProps> = ({ page }) => {
  if (!page?.components || page.components.length === 0) return null;

  return (
    <div className="page-root fade-in" data-page-id={page.pageId}>
      {page.title && (
        <h1 className="page-title">{page.title}</h1>
      )}
      {page.components.map((component: Component, idx: number) => (
        <ComponentRenderer key={component.id || idx} component={component} />
      ))}
    </div>
  );
};
