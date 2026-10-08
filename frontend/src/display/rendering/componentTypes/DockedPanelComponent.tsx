import React, { useEffect, useRef, useState } from "react";
import ComponentRenderer from "../ComponentRenderer";
import { BaseComponentProps } from "./TextComponent";

// A CONTAINER with a header bar that folds it away. Two placements, chosen
// by the server's data-docked attribute: "true" pins it to the bottom-right
// corner over the page; "side" pins it to the right EDGE of the window, the
// full height, always in place while the page scrolls beside it -- a
// sidebar, the way a chat app keeps its panel. The page is pushed left by
// the panel's width so nothing sits under it. Folded, it is a thin strip.
// data-docked-title names it; everything inside is rendered as usual.
//
// Whether it is folded is the reader's business, not the server's: it lives
// in localStorage, so a page rebuilt every two seconds while the model thinks
// does not pop the panel open again, and a reload keeps it where it was.
const storageKey = (id: string) => `docked:${id}:collapsed`;

const SIDE_WIDTH = "24rem";
const SIDE_FOLDED_WIDTH = "2.75rem";

const DockedPanelComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const id = component.id || "dockedPanel";
  const attributes = (component as any).attributes || {};
  const title: string = attributes["data-docked-title"] || "Panel";
  const side = attributes["data-docked"] === "side";
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

  // The page makes room for the sidebar: body padding equal to its width,
  // so the columns lay out beside it rather than under it. Undone when the
  // panel leaves the page.
  useEffect(() => {
    if (!side) return;
    const previous = document.body.style.paddingRight;
    document.body.style.paddingRight = collapsed ? SIDE_FOLDED_WIDTH : SIDE_WIDTH;
    return () => {
      document.body.style.paddingRight = previous;
    };
  }, [side, collapsed]);

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
      style={
        side
          ? {
              position: "fixed",
              top: 0,
              right: 0,
              bottom: 0,
              width: collapsed ? SIDE_FOLDED_WIDTH : SIDE_WIDTH,
              maxWidth: "80vw",
              display: "flex",
              flexDirection: "column",
              zIndex: 40,
              background: "var(--panel)",
              borderLeft: "1px solid var(--line)",
              overflow: "hidden",
            }
          : {
              position: "fixed",
              right: "1rem",
              bottom: "1rem",
              width: collapsed ? "16rem" : "27rem",
              maxWidth: "calc(100vw - 2rem)",
              maxHeight: "75vh",
              display: "flex",
              flexDirection: "column",
              zIndex: 50,
              background: "var(--panel)",
              border: "1px solid var(--line)",
              borderRadius: "var(--radius)",
              boxShadow: "0 10px 30px rgba(0, 0, 0, 0.5)",
              overflow: "hidden",
            }
      }
    >
      <button
        type="button"
        onClick={toggle}
        aria-expanded={!collapsed}
        style={{
          display: "flex",
          alignItems: "center",
          gap: "0.5rem",
          width: "100%",
          padding: side && collapsed ? "10px 0" : "10px 14px",
          justifyContent: side && collapsed ? "center" : "space-between",
          background: "var(--panel-2)",
          color: "var(--text)",
          border: "none",
          borderBottom: collapsed ? "none" : "1px solid var(--line)",
          borderRadius: 0,
          cursor: "pointer",
          font: "inherit",
          fontSize: "13px",
          fontWeight: 600,
          textAlign: "left",
        }}
      >
        {!(side && collapsed) && <span>{title}</span>}
        <span aria-hidden="true" title={side && collapsed ? title : undefined}>
          {side ? (collapsed ? "◀" : "▶") : collapsed ? "▲" : "▼"}
        </span>
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
            // In a side column the body takes the rest of the window height.
            ...(side ? { flex: "1 1 auto" } : {}),
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
