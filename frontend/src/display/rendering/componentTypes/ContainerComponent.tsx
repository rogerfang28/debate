import React, { useEffect, useState } from "react";
import ComponentRenderer from "../ComponentRenderer";
import { BaseComponentProps } from "./TextComponent";
import handleEvent from "../../events/handleEvent";
import { RegionContext, readStored, writeStored } from "./shellContext";

// ---- Page layout: the server marks the parts, the client lays them out ----
//
// data-layout="shell" | "document" on a page's root: an app shell (header,
// then regions that fill the window and scroll on their own) or a document
// (header, then one scrolling reading column with the chat beside it).
// data-region="header" | "inspector" | "main" | "chat" | "page": which part
// of the layout a container is. The container whose children are the
// regions is the region grid. index.css has the geometry.
// data-section="<title>": a panel that folds under a quiet header.
// data-view="graph" | "text": the two ways the main region shows the
// argument; the region's bar picks one.
//
// Folds and the Graph | Text choice are the reader's business, kept in
// localStorage like the chat's fold, never sent to the server.

const GRID_REGIONS = new Set(["inspector", "main", "chat", "page"]);
const VIEW_KEY = "shell:view";

function dataAttributes(attributes: Record<string, string>): Record<string, string> {
  const out: Record<string, string> = {};
  for (const key of Object.keys(attributes)) {
    if (key.startsWith("data-")) out[key] = attributes[key];
  }
  return out;
}

const renderChildren = (nested: any[]) =>
  nested.map((child: any, idx: number) => (
    <ComponentRenderer key={child.id || idx} component={child} />
  ));

// A folding section: a caption and a chevron over the panel's body. Open by
// default only for "Who owes a reply" -- the one thing a reader needs at a
// glance; the rest wait to be asked for.
const SectionPanel: React.FC<{ component: any; className?: string; style?: React.CSSProperties; title: string }> = ({
  component, className, style, title,
}) => {
  const nested = component.children || component.components || [];
  const key = `section:${title}:open`;
  const [open, setOpen] = useState<boolean>(
    () => readStored(key, title === "Who owes a reply" ? "1" : "0") === "1"
  );
  const toggle = () =>
    setOpen((was) => {
      writeStored(key, was ? "0" : "1");
      return !was;
    });
  return (
    <div id={component.id} className={`section${open ? " open" : ""} ${className || ""}`} data-section={title}>
      <button type="button" className="section-head" onClick={toggle} aria-expanded={open}>
        <span className="caption">{title}</span>
        <span aria-hidden="true">{open ? "\u25BE" : "\u25B8"}</span>
      </button>
      {open && (
        <div className="section-body" style={style}>
          {renderChildren(nested)}
        </div>
      )}
    </div>
  );
};

// The main region: a bar with the Graph | Text toggle over the views, one
// shown at a time. Below 1100px the bar also opens the inspector drawer.
const MainRegion: React.FC<{ component: any; className?: string; style?: React.CSSProperties }> = ({
  component, className, style,
}) => {
  const nested = component.children || component.components || [];
  const [view, setView] = useState<string>(() => (readStored(VIEW_KEY, "graph") === "text" ? "text" : "graph"));
  const pick = (next: string) => {
    writeStored(VIEW_KEY, next);
    setView(next);
  };
  const hasText = nested.some((c: any) => c.attributes?.["data-view"] === "text");
  // The drawer's open state is a mark on <html>; the inspector reads it in CSS.
  useEffect(() => () => document.documentElement.removeAttribute("data-inspector-open"), []);
  return (
    <div id={component.id} className={`region main ${className || ""}`} data-region="main" data-show={view} style={style}>
      <div className="region-bar">
        {hasText && (
          <div className="view-toggle" role="tablist">
            <button type="button" role="tab" aria-selected={view === "graph"} className={`pill${view === "graph" ? " on" : ""}`} onClick={() => pick("graph")}>
              Graph
            </button>
            <button type="button" role="tab" aria-selected={view === "text"} className={`pill${view === "text" ? " on" : ""}`} onClick={() => pick("text")}>
              Text
            </button>
          </div>
        )}
        {/* At the right end, clear of the drawer it opens over the left. */}
        <button
          type="button"
          className="btn sm drawer-toggle"
          onClick={() => document.documentElement.toggleAttribute("data-inspector-open")}
        >
          Selected claim
        </button>
      </div>
      <RegionContext.Provider value="main">{renderChildren(nested)}</RegionContext.Provider>
    </div>
  );
};

// Any other region. The inspector pins its first child (the selected claim)
// and scrolls the rest beneath it.
function renderRegion(region: string, component: any, className: string | undefined, style: React.CSSProperties | undefined) {
  const nested = component.children || component.components || [];
  const cls = `region ${region} ${className || ""}`;
  let body: React.ReactNode;
  if (region === "inspector" && nested.length > 0) {
    body = (
      <>
        <div className="region-pin">{renderChildren(nested.slice(0, 1))}</div>
        {nested.length > 1 && <div className="region-scroll">{renderChildren(nested.slice(1))}</div>}
      </>
    );
  } else {
    body = renderChildren(nested);
  }
  return (
    <div id={component.id} className={cls} data-region={region} style={style}>
      <RegionContext.Provider value={region}>{body}</RegionContext.Provider>
    </div>
  );
}

// Two opt-in behaviours, chosen by the server through attributes:
//
// data-click="true": the whole container is one press, sent as an onClick
// from the container's own id (a card on the home shelf opens its debate).
// A press on a button or field inside it is that control's own, not the
// container's.
//
// data-fold="closed" | "open": the children sit under a show/hide header
// titled data-fold-title. Opening and closing is the reader's business, so it
// never goes to the server; the page keeps it while it is rebuilt.
const ContainerComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const isFlex = className?.includes('flex');
  const isGrid = className?.includes('grid');
  // Support both "children" (from protobuf) and "components" (from pbtxt) for nesting
  const nested = component.children || component.components || [];
  const attributes = (component as any).attributes || {};
  const clickable = attributes["data-click"] === "true";
  const fold = attributes["data-fold"];

  const [open, setOpen] = useState<boolean>(fold === "open");
  const [hover, setHover] = useState<boolean>(false);

  const press = (e: React.MouseEvent | React.KeyboardEvent) => {
    const target = e.target as HTMLElement;
    if (target.closest("button, input, textarea, select, a")) return;
    handleEvent(e as any, component, "onClick", component.id || "unknown-container");
  };

  const region: string | undefined = attributes["data-region"];
  if (attributes["data-section"]) {
    return <SectionPanel component={component} className={className} style={style} title={attributes["data-section"]} />;
  }
  if (region === "main") {
    return <MainRegion component={component} className={className} style={style} />;
  }
  if (region && GRID_REGIONS.has(region)) {
    return renderRegion(region, component, className, style);
  }
  // The container holding the regions is the grid they stand in.
  const regionGrid = nested.some((c: any) => GRID_REGIONS.has(c.attributes?.["data-region"]));
  // Laid out by index.css: no default flex column or clipping from here.
  const placed = regionGrid || !!attributes["data-layout"] || !!attributes["data-view"] || region === "header";

  if (fold === "open" || fold === "closed") {
    return (
      <div id={component.id} className={className} style={{ ...style, overflow: "visible" }}>
        <button
          type="button"
          onClick={() => setOpen((was) => !was)}
          aria-expanded={open}
          style={{
            display: "flex",
            alignItems: "center",
            gap: "0.5rem",
            background: "transparent",
            border: "none",
            borderTop: "1px solid var(--line)",
            borderRadius: 0,
            justifyContent: "flex-start",
            width: "100%",
            padding: "0.75rem 0 0.25rem",
            color: "var(--text-2)",
            fontSize: "12px",
            fontWeight: 600,
            cursor: "pointer",
            textAlign: "left",
          }}
        >
          <span aria-hidden="true">{open ? "▼" : "▶"}</span>
          <span>{attributes["data-fold-title"] || "More"}</span>
        </button>
        {open &&
          nested.map((child: any, idx: number) => (
            <ComponentRenderer key={child.id || idx} component={child} />
          ))}
      </div>
    );
  }

  return (
    <div
      id={component.id}
      className={regionGrid ? `shell-regions ${className || ""}` : className}
      {...dataAttributes(attributes)}
      {...(clickable
        ? {
            role: "button",
            tabIndex: 0,
            onClick: press,
            onKeyDown: (e: React.KeyboardEvent) => {
              if (e.key === "Enter" || e.key === " ") {
                e.preventDefault();
                press(e);
              }
            },
            onMouseEnter: () => setHover(true),
            onMouseLeave: () => setHover(false),
          }
        : {})}
      style={{
        ...(!isFlex && !isGrid && !placed ? {
          display: 'flex',
          flexDirection: 'column' as const,
          gap: '0.75rem',
        } : {}),
        ...style,
        ...(placed ? {} : { overflow: style?.overflow || 'hidden' }),
        ...(clickable
          ? {
              cursor: "pointer",
              transition: "box-shadow 120ms",
              // A ring rather than a border colour: the server sets the
              // border as a shorthand, which a longhand would fight.
              boxShadow: hover ? "0 0 0 1px var(--accent)" : "none",
            }
          : {}),
      }}
    >
      {nested.map((child: any, idx: number) => (
        <ComponentRenderer key={child.id || idx} component={child} />
      ))}
    </div>
  );
};

export default ContainerComponent;
