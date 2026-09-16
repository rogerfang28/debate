import React, { useEffect, useRef, useState } from "react";
import ComponentRenderer from "../ComponentRenderer";
import { BaseComponentProps } from "./TextComponent";

// A CONTAINER pinned to the bottom-right corner, with a header bar that folds
// it away. The server marks a container with data-docked="true" and a
// data-docked-title; everything inside is rendered as usual.
//
// Whether it is folded is the reader's business, not the server's: it lives
// in localStorage, so a page rebuilt every two seconds while the model thinks
// does not pop the panel open again, and a reload keeps it where it was.
const storageKey = (id: string) => `docked:${id}:collapsed`;

const DockedPanelComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const id = component.id || "dockedPanel";
  const attributes = (component as any).attributes || {};
  const title: string = attributes["data-docked-title"] || "Panel";
  const nested = component.children || component.components || [];

  const [collapsed, setCollapsed] = useState<boolean>(() => {
    try {
      return localStorage.getItem(storageKey(id)) === "1";
    } catch {
      return false;
    }
  });
  const toggle = () => {
    setCollapsed((was) => {
      try {
        localStorage.setItem(storageKey(id), was ? "0" : "1");
      } catch {
        // Storage may be unavailable; the panel still toggles for this page.
      }
      return !was;
    });
  };

  // Keep the newest turn in view: the body scrolls to its end when a turn is
  // added -- not on every rebuild, or the reader could never scroll back up
  // while the page polls.
  const bodyRef = useRef<HTMLDivElement>(null);
  const lastCount = useRef<number>(-1);
  useEffect(() => {
    if (collapsed || !bodyRef.current) return;
    const count = bodyRef.current.querySelectorAll("[id^=chatWho_]").length;
    if (count !== lastCount.current) {
      lastCount.current = count;
      bodyRef.current.scrollTop = bodyRef.current.scrollHeight;
    }
  });

  return (
    <div
      id={id}
      style={{
        position: "fixed",
        right: "1rem",
        bottom: "1rem",
        width: collapsed ? "16rem" : "27rem",
        maxWidth: "calc(100vw - 2rem)",
        maxHeight: "75vh",
        display: "flex",
        flexDirection: "column",
        zIndex: 50,
        background: "#1f2937",
        border: "1px solid #374151",
        borderRadius: "0.75rem",
        boxShadow: "0 10px 30px rgba(0, 0, 0, 0.5)",
        overflow: "hidden",
      }}
    >
      <button
        type="button"
        onClick={toggle}
        aria-expanded={!collapsed}
        style={{
          display: "flex",
          alignItems: "center",
          justifyContent: "space-between",
          gap: "0.5rem",
          width: "100%",
          padding: "0.6rem 0.9rem",
          background: "#312e81",
          color: "#e0e7ff",
          border: "none",
          borderBottom: collapsed ? "none" : "1px solid #374151",
          cursor: "pointer",
          font: "inherit",
          fontSize: "0.85rem",
          fontWeight: 600,
          textAlign: "left",
        }}
      >
        <span>{title}</span>
        <span aria-hidden="true">{collapsed ? "▲" : "▼"}</span>
      </button>
      {!collapsed && (
        <div
          ref={bodyRef}
          className={className}
          style={{
            ...style,
            // Block, not flex: a flex column with a capped height SHRINKS its
            // children to fit, which clipped every chat turn to one line.
            // The body scrolls; its children keep their size.
            display: "block",
            overflowY: "auto",
            minHeight: 0,
          }}
        >
          {nested.map((child: any, idx: number) => (
            <ComponentRenderer key={child.id || idx} component={child} />
          ))}
        </div>
      )}
    </div>
  );
};

export default DockedPanelComponent;
