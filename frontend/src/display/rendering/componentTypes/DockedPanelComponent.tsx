import React, { useContext, useEffect, useRef, useState } from "react";
import ComponentRenderer from "../ComponentRenderer";
import { BaseComponentProps } from "./TextComponent";
import { RegionContext } from "./shellContext";
import { childKeys } from "../childKeys";

// A CONTAINER with a header bar that folds it away. Where it stands is the
// client's call, from the server's data-docked attribute and where the panel
// sits:
//   "side" in a chat region      -> region: it IS the page's chat region,
//                                   the window's height, its turns scrolling
//                                   above an ask box pinned at the bottom.
//                                   Folded, the region narrows to a strip.
//                                   Below 1100px it falls back to the corner.
//   anything else ("true")       -> corner: a box pinned to the bottom-right
//                                   corner over the page.
// data-docked-title names it; everything inside is rendered as usual. The
// look is index.css's (.docked).
//
// Whether it is folded is the reader's business, not the server's: it lives
// in localStorage, so a page rebuilt every two seconds while the model thinks
// does not pop the panel open again, and a reload keeps it where it was.
const storageKey = (id: string) => `docked:${id}:collapsed`;

const DockedPanelComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const id = component.id || "dockedPanel";
  const attributes = (component as any).attributes || {};
  const title: string = attributes["data-docked-title"] || "Panel";
  const region = useContext(RegionContext);
  const placement = attributes["data-docked"] === "side" && region === "chat" ? "region" : "corner";
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

  // Keep the newest turn in view: the scroller goes to its end when a turn
  // is added -- not on every rebuild, or the reader could never scroll back
  // up while the page polls.
  const scrollRef = useRef<HTMLDivElement>(null);
  const lastCount = useRef<number>(-1);
  useEffect(() => {
    if (collapsed || !scrollRef.current) return;
    const count = scrollRef.current.querySelectorAll("[id^=chatWho_]").length;
    if (count !== lastCount.current) {
      lastCount.current = count;
      scrollRef.current.scrollTop = scrollRef.current.scrollHeight;
    }
  });

  const edge = placement === "region";
  const chevron = edge ? (collapsed ? "◀" : "▶") : collapsed ? "▲" : "▼";

  // As a region the conversation scrolls on its own and the rest -- the ask
  // box, its buttons, a note -- stands under it.
  let body: React.ReactNode;
  if (placement === "region") {
    const turns = nested.filter((child: any) => child.id === "chatTranscript");
    const ask = nested.filter((child: any) => child.id !== "chatTranscript");
    body = (
      <div className="docked-body" style={style}>
        <div ref={scrollRef} className="docked-turns">
          {turns.map((child: any, idx: number) => (
            <ComponentRenderer key={childKeys(id, turns)[idx]} component={child} />
          ))}
        </div>
        <div className="docked-ask">
          {ask.map((child: any, idx: number) => (
            <ComponentRenderer key={childKeys(id, ask)[idx]} component={child} />
          ))}
        </div>
      </div>
    );
  } else {
    body = (
      <div ref={scrollRef} className={`docked-body ${className || ""}`} style={style}>
        {nested.map((child: any, idx: number) => (
          <ComponentRenderer key={childKeys(id, nested)[idx]} component={child} />
        ))}
      </div>
    );
  }

  return (
    <div id={id} className={`docked ${placement}${collapsed ? " folded" : ""}`} data-folded={collapsed ? "true" : "false"}>
      <button type="button" className="docked-head" onClick={toggle} aria-expanded={!collapsed} title={collapsed ? title : undefined}>
        <span className="docked-title">{title}</span>
        <span aria-hidden="true" className="docked-chevron">{chevron}</span>
      </button>
      {!collapsed && body}
    </div>
  );
};

export default DockedPanelComponent;
