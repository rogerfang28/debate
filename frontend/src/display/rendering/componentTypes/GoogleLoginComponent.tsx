import React, { useEffect, useRef } from "react";
import handleEvent from "../../events/handleEvent";
import { BaseComponentProps } from "./TextComponent";

// eslint-disable-next-line @typescript-eslint/no-explicit-any
declare global {
  interface Window {
    google?: any;
    onGoogleLoginSuccess?: (credential: string) => void;
    onGoogleLoginError?: () => void;
  }
}

interface GoogleLoginComponentProps {
  component: BaseComponentProps;
  className?: string;
  style?: React.CSSProperties;
}

const GoogleLoginComponent: React.FC<GoogleLoginComponentProps> = ({ component }) => {
  const containerRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    // Load Google Identity Services script
    if (!window.google) {
      const script = document.createElement("script");
      script.src = "https://accounts.google.com/gsi/client";
      script.async = true;
      script.defer = true;
      script.onload = () => {
        if (containerRef.current && window.google) {
          const clientId = document.documentElement.getAttribute("data-google-client-id") || "";
          if (clientId) {
            window.google.accounts.id.initialize({
              client_id: clientId,
              callback: (response: { credential: string }) => {
                // Send the Google ID token to the backend
                window.googleLoginCallback?.(response.credential);
              },
              error_callback: () => {
                console.error("[GoogleAuth] Google sign-in error");
              },
            });

            window.google.accounts.id.renderButton(
              containerRef.current,
              {
                theme: "filled_blue",
                size: "large",
                shape: "rounded_rect",
                text: "continue_with",
                width: "250",
              }
            );
          }
        }
      };
      document.head.appendChild(script);
    }
  }, []);

  return (
    <div
      ref={containerRef}
      id={component.id}
      className="google-login-container"
      style={{ display: "flex", justifyContent: "center", padding: "8px 0" }}
    />
  );
};

export default GoogleLoginComponent;
