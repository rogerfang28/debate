import React, { useEffect, useState, useCallback, useRef } from "react";
import { PageRenderer } from "./rendering/PageRenderer.tsx";
import postClientMessageToCPP from "../backendCommunicator/postClientMessageToCPP.ts";
import { fromJson } from "@bufbuild/protobuf";
import { PageSchema } from "../../../src/gen/ts/layout_pb.ts";

// eslint-disable-next-line @typescript-eslint/no-explicit-any
declare global {
  interface Window {
    reloadPage?: () => Promise<void>;
    googleLoginCallback?: (credential: string) => void;
  }
}

interface PageData {
  [key: string]: any;
}

const Renderer: React.FC = () => {
  const [data, setData] = useState<PageData | null>(null);
  const [loading, setLoading] = useState<boolean>(false);
  const [error, setError] = useState<string | null>(null);
  const fileInputRef = useRef<HTMLInputElement>(null);
  const dataRef = useRef<PageData | null>(null);

  // Keep ref in sync with state for the async callback
  useEffect(() => {
    dataRef.current = data;
  }, [data]);

  const reloadPage = useCallback(async (): Promise<void> => {
    try {
      setLoading(true);
      setError(null);
      const info: PageData | null = await postClientMessageToCPP({});
      if (info) {
        setData(info);
      }
    } catch (err: unknown) {
      console.error("reloadPage: Failed to load data", err);
      setError("Failed to refresh page");
    } finally {
      setLoading(false);
    }
  }, []);

  // Expose global reload function for handleEvent.ts
  useEffect(() => {
    window.reloadPage = reloadPage;
    return () => {
      window.reloadPage = undefined;
    };
  }, [reloadPage]);

  // Fetch page on mount only — subsequent refreshes are event-driven via reloadPage()
  useEffect(() => {
    let mounted = true;

    async function fetchData(): Promise<void> {
      if (!mounted) return;
      try {
        setLoading(true);
        const info: PageData | null = await postClientMessageToCPP({});
        if (!mounted) return;
        if (info) {
          setData(info);
          setError(null);
        }
      } catch (err: unknown) {
        if (!mounted) return;
        console.warn("Renderer: Failed to load data", err);
        if (!dataRef.current) {
          setError("Connecting to server...");
        }
      } finally {
        if (mounted) setLoading(false);
      }
    }

    fetchData();

    return () => {
      mounted = false;
    };
  }, []);

  // Load from JSON file
  const handleLoadFromFile = useCallback(() => {
    fileInputRef.current?.click();
  }, []);

  const handleFileChange = useCallback((e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = (event) => {
      try {
        const jsonStr = event.target?.result as string;
        const jsonObj = JSON.parse(jsonStr);
        const page = fromJson(PageSchema, jsonObj);
        setData(page as unknown as PageData);
        setError(null);
      } catch (err: unknown) {
        console.error("Failed to parse JSON file", err);
        setError("Failed to parse JSON file: " + (err instanceof Error ? err.message : String(err)));
      }
    };
    reader.onerror = () => {
      setError("Failed to read file");
    };
    reader.readAsText(file);

    // Reset input so the same file can be selected again
    e.target.value = "";
  }, []);

  return (
    <div className="min-h-screen fade-in" style={{ background: 'var(--surface-bg)', color: 'var(--text-primary)' }}>
      {/* Load from file — always visible, fixed bottom-right */}
      <input
        ref={fileInputRef}
        type="file"
        accept=".json"
        style={{ display: 'none' }}
        onChange={handleFileChange}
      />

      <button
        onClick={handleLoadFromFile}
        className="fixed bottom-4 right-4 z-50 flex items-center gap-2 text-xs font-medium px-3 py-2 rounded-lg shadow-lg transition-opacity hover:opacity-80"
        style={{
          background: 'var(--surface-elevated)',
          border: '1px solid var(--border-default)',
          color: 'var(--text-secondary)',
        }}
        title="Load layout from JSON file"
      >
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
          <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4M17 8l-5-5-5 5M12 3v12" />
        </svg>
        Load File
      </button>

      {loading && (
        <div
          className="fixed top-4 right-4 flex items-center gap-2 text-xs font-medium z-50 px-3 py-2 rounded-lg shadow-lg"
          style={{
            background: 'var(--surface-elevated)',
            border: '1px solid var(--border-default)',
            color: 'var(--text-secondary)',
          }}
        >
          <div
            className="w-3 h-3 rounded-full border-2 border-transparent animate-spin"
            style={{ borderTopColor: 'var(--accent-indigo)' }}
          />
          Refreshing…
        </div>
      )}

      {/* Loading state — no data yet */}
      {loading && !data && (
        <div className="flex flex-col items-center justify-center h-screen">
          <div className="flex flex-col items-center gap-4">
            <div
              className="animate-spin rounded-full h-10 w-10 border-2 border-transparent"
              style={{
                borderTopColor: 'var(--accent-indigo)',
                borderRightColor: 'var(--accent-indigo)',
              }}
            />
            <p className="text-sm" style={{ color: 'var(--text-muted)' }}>
              Loading page...
            </p>
          </div>
        </div>
      )}

      {/* Error state — no data yet */}
      {error && !data && (
        <div className="flex flex-col items-center justify-center h-screen">
          <div className="flex flex-col items-center gap-4 text-center">
            <div
              className="w-12 h-12 rounded-full flex items-center justify-center"
              style={{ background: 'var(--surface-card)', border: '1px solid var(--border-default)' }}
            >
              <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="var(--text-muted)" strokeWidth="2">
                <path d="M12 2v4M12 18v4M4.93 4.93l2.83 2.83M16.24 16.24l2.83 2.83M2 12h4M18 12h4M4.93 19.07l2.83-2.83M16.24 7.76l2.83-2.83" />
              </svg>
            </div>
            <p className="text-sm" style={{ color: 'var(--text-secondary)' }}>{error}</p>
            <button onClick={reloadPage} className="text-xs px-4 py-2 rounded-md" style={{ background: 'var(--surface-card)', border: '1px solid var(--border-default)', color: 'var(--text-accent)' }}>
              Retry
            </button>
          </div>
        </div>
      )}

      {/* Data loaded — render page */}
       {data && <PageRenderer page={data} />}

       {/* Google Sign-In SDK — inject when on login page */}
       <GoogleSdkInjector />

      {/* Waiting for data — no error, no loading, no data */}
      {!loading && !error && !data && (
        <div className="flex flex-col items-center justify-center h-screen gap-4">
          <p className="text-sm" style={{ color: 'var(--text-muted)' }}>Waiting for page data…</p>
        </div>
      )}
    </div>
  );
};

// eslint-disable-next-line react/forward-ref-ref
const GoogleSdkInjector: React.FC = () => {
  const dataRef = useRef<PageData | null>(null);
  const initializedRef = useRef(false);

  useEffect(() => {
    dataRef.current = data;
  }, [data]);

  useEffect(() => {
     if (initializedRef.current) return;

     // Check if the current page has the Google login button
     const pageData = dataRef.current;
     if (!pageData || !pageData.components) return;

     const hasGoogleButton = pageData.components.some(
       (comp: PageData) => comp.id === "googleLoginButton"
     );
     if (!hasGoogleButton) return;

     const clientId = import.meta.env.VITE_GOOGLE_CLIENT_ID || import.meta.env.GOOGLE_CLIENT_ID || "";
     if (!clientId) return;

     initializedRef.current = true;

     // Set data attribute for Google JS SDK to read
     document.documentElement.setAttribute("data-google-client-id", clientId);

     // Load Google Identity Services
     const loadGoogleSDK = () => {
       if (!window.google) {
         const script = document.createElement("script");
         script.src = "https://accounts.google.com/gsi/client";
         script.async = true;
         script.defer = true;
         script.onload = () => {
           if (window.google) {
             initializeGoogle(clientId);
           }
         };
         script.onerror = () => {
           console.error("[GoogleAuth] Failed to load Google JS SDK");
         };
         document.head.appendChild(script);
       } else {
         initializeGoogle(clientId);
       }
     };

     const initializeGoogle = (cid: string) => {
       window.google.accounts.id.initialize({
         client_id: cid,
         callback: (response: { credential: string }) => {
           console.log("[GoogleAuth] Got credential, sending to backend...");
           window.googleLoginCallback?.(response.credential);
         },
         error_callback: () => {
           console.error("[GoogleAuth] Google sign-in error");
         },
         auto_select: false,
       });

       // Wait for the container DOM element to exist, then render button
       const tryRender = () => {
         const container = document.getElementById("googleLoginButton");
         if (container) {
           window.google.accounts.id.renderButton(container, {
             theme: "outline",
             size: "large",
             shape: "rectangular",
             width: 300,
             text: "continue_with",
           });
         } else {
           requestAnimationFrame(tryRender);
         }
       };
       requestAnimationFrame(tryRender);
     };

     loadGoogleSDK();
   }, []);

  return null;
};

export default Renderer;
