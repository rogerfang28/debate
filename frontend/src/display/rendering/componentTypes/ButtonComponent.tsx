import React, { useState } from "react";
import { BaseComponentProps } from "./TextComponent";
import handleEvent from "../../events/handleEvent";

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
     fireClick(e);
  };

  const isSmall = className?.includes('text-xs') || className?.includes('text-sm');
  const isLarge = className?.includes('text-lg') || className?.includes('text-xl');
  const isRounded = className?.includes('rounded-full');

  return (
    <>
      <button
        id={component.id}
        className={className}
        style={{
          ...style,
          padding: isSmall
            ? '0.375rem 0.875rem'
            : isLarge
              ? '0.75rem 1.5rem'
              : '0.625rem 1.25rem',
          borderRadius: isRounded ? '9999px' : 'var(--radius-md)',
          fontSize: isSmall
            ? '0.8125rem'
            : isLarge
              ? '1rem'
              : '0.875rem',
          fontWeight: 500,
          maxWidth: '100%',
          overflow: 'hidden',
          textOverflow: 'ellipsis',
          whiteSpace: 'nowrap',
          display: 'inline-flex',
          justifyContent: 'center',
          alignItems: 'center',
        }}
        onClick={handleClick}
      >
        {component.text}
      </button>
      {showConcedeConfirm && (
        <div
          style={{
            position: "fixed",
            inset: 0,
            background: "rgba(0, 0, 0, 0.6)",
            display: "flex",
            alignItems: "center",
            justifyContent: "center",
            zIndex: 1000,
          }}
          onClick={() => setShowConcedeConfirm(false)}
        >
          <div
            style={{
              background: "#1f2937",
              border: "1px solid #374151",
              borderRadius: "0.5rem",
              padding: "1.5rem",
              maxWidth: "24rem",
              width: "90%",
              boxShadow: "0 10px 25px rgba(0, 0, 0, 0.5)",
            }}
            onClick={(e) => e.stopPropagation()}
          >
            <div style={{ color: "#ffffff", fontSize: "1rem", fontWeight: 600, marginBottom: "0.5rem" }}>
              Concede this challenge?
            </div>
            <div style={{ color: "#9ca3af", fontSize: "0.875rem", marginBottom: "1.25rem" }}>
              This will mark your claim as false and cannot be undone.
            </div>
            <div style={{ display: "flex", justifyContent: "flex-end", gap: "0.5rem" }}>
              <button
                onClick={() => setShowConcedeConfirm(false)}
                style={{
                  padding: "0.5rem 1rem",
                  borderRadius: "0.375rem",
                  fontSize: "0.875rem",
                  fontWeight: 500,
                  background: "#374151",
                  color: "#ffffff",
                  border: "none",
                  cursor: "pointer",
                }}
              >
                Cancel
              </button>
              <button
                onClick={(e) => {
                  setShowConcedeConfirm(false);
                  fireClick(e);
                }}
                style={{
                  padding: "0.5rem 1rem",
                  borderRadius: "0.375rem",
                  fontSize: "0.875rem",
                  fontWeight: 500,
                  background: "#dc2626",
                  color: "#ffffff",
                  border: "none",
                  cursor: "pointer",
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
