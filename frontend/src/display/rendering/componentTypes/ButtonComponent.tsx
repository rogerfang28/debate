import React, { useState } from "react";
import { BaseComponentProps } from "./TextComponent";
import handleEvent from "../../events/handleEvent";
import { layoutClasses } from "../buildClassName";

// The theme's button (theme.html .btn), picked from the server's class hints.
// The server says "bg-blue-600 text-sm"; the theme says what that looks like:
//   bg-blue / bg-indigo        -> primary, the one filled button on a screen
//   bg-red                     -> danger (red words, no fill)
//   bg-gray-800 / transparent  -> quiet (no border)
//   no colour at all           -> quiet (its inline css usually paints it)
//   anything else              -> outlined
//   rounded-full               -> a pill
//   text-xs / text-sm          -> small
// The paint classes are dropped; layout classes (w-full, self-start) stay.
export function buttonClassName(className?: string): string {
  const c = className || "";
  const small = /(^|\s)text-(xs|sm)(\s|$)/.test(c);
  let look: string;
  if (/(^|\s)rounded-full(\s|$)/.test(c)) {
    look = "pill";
  } else if (/(^|\s)bg-(blue|indigo)-/.test(c)) {
    look = "btn primary";
  } else if (/(^|\s)bg-red-/.test(c)) {
    look = "btn danger";
  } else if (/(^|\s)bg-(gray-800|transparent)(\s|$)/.test(c) || !/(^|\s)bg-/.test(c)) {
    look = "btn quiet";
  } else {
    look = "btn";
  }
  if (small && look !== "pill") look += " sm";
  const layout = layoutClasses(c);
  return layout ? `${look} ${layout}` : look;
}

const ButtonComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const [showConcedeConfirm, setShowConcedeConfirm] = useState(false);

  const fireClick = (e: React.MouseEvent | null) => {
    console.log(`Button "${component.id}" clicked`);
    handleEvent(
      e as any,
      component,
      'onClick',
      component.id || 'unknown-button'
    );
  };

  const handleClick = (e: React.MouseEvent) => {
     // Google login button — handled by Google JS SDK callback instead
     if (component.id === "googleLoginButton") {
       // The Google SDK injects its own button inside this container
       // Do not trigger normal click handling
       return;
     }
     // Conceding is irreversible (marks the claim false and cannot be undone),
     // so confirm before sending the event.
     if (component.id?.startsWith("concedeChallengeButton_")) {
       setShowConcedeConfirm(true);
       return;
     }
     // Deleting a debate cannot be undone either, and on the home shelf its
     // control sits on a card that is itself a press.
     if (component.id?.startsWith("deleteDebateButton_") &&
         !window.confirm("Delete this debate? This cannot be undone.")) {
       return;
     }
     fireClick(e);
  };

  return (
    <>
      <button
        id={component.id}
        className={buttonClassName(className)}
        style={{
          maxWidth: '100%',
          overflow: 'hidden',
          textOverflow: 'ellipsis',
          ...style,
        }}
        onClick={handleClick}
      >
        {component.text}
      </button>
      {showConcedeConfirm && (
        <div
          className="modal-overlay"
          style={{ zIndex: 1000 }}
          onClick={() => setShowConcedeConfirm(false)}
        >
          <div
            className="modal-content"
            style={{ maxWidth: "24rem", width: "90%" }}
            onClick={(e) => e.stopPropagation()}
          >
            <div className="t-h3" style={{ marginBottom: "0.5rem" }}>
              Concede this challenge?
            </div>
            <div className="t-body-2" style={{ color: "var(--text-2)", marginBottom: "1.25rem" }}>
              This will mark your claim as false and cannot be undone.
            </div>
            <div style={{ display: "flex", justifyContent: "flex-end", gap: "0.5rem" }}>
              <button className="btn quiet" onClick={() => setShowConcedeConfirm(false)}>
                Cancel
              </button>
              <button
                className="btn danger"
                onClick={(e) => {
                  setShowConcedeConfirm(false);
                  fireClick(e);
                }}
              >
                Concede
              </button>
            </div>
          </div>
        </div>
      )}
    </>
  );
};

export default ButtonComponent;
