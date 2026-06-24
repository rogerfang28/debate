import React, { useEffect, useState, useCallback } from "react";
import { PageRenderer } from "./rendering/PageRenderer.tsx";
import postClientMessageToCPP from "../backendCommunicator/postClientMessageToCPP.ts";

interface PageData {
  [key: string]: any;
}

declare global {
  interface Window {
    reloadPage?: () => Promise<void>;
  }
}

const Renderer: React.FC = () => {
  const [data, setData] = useState<PageData | null>(null);
  const [loading, setLoading] = useState<boolean>(false);
  const [error, setError] = useState<string | null>(null);

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

  useEffect(() => {
    let intervalId: NodeJS.Timeout | undefined;
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
          if (intervalId) clearInterval(intervalId);
        } else {
          setError("Connecting to server...");
        }
      } catch (err: unknown) {
        if (!mounted) return;
        console.warn("Renderer: Failed to load data, retrying...", err);
        setError("Connecting to server...");
      } finally {
        if (mounted) setLoading(false);
      }
    }

    fetchData();
    intervalId = setInterval(fetchData, 3000);

    return () => {
      mounted = false;
      if (intervalId) clearInterval(intervalId);
    };
  }, []);

  // Loading state
  if (loading && !data) {
    return (
      <div className="flex flex-col items-center justify-center h-screen" style={{ background: 'var(--surface-bg)' }}>
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
    );
  }

  // Error state with retry
  if (error && !data) {
    return (
      <div className="flex flex-col items-center justify-center h-screen" style={{ background: 'var(--surface-bg)' }}>
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
    );
  }

  return (
    <div className="min-h-screen fade-in" style={{ background: 'var(--surface-bg)', color: 'var(--text-primary)' }}>
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
      {data ? (
        <PageRenderer page={data} />
      ) : (
        <div className="flex flex-col items-center justify-center h-screen gap-4" style={{ background: 'var(--surface-bg)' }}>
          <p className="text-sm" style={{ color: 'var(--text-muted)' }}>Waiting for page data…</p>
        </div>
      )}
    </div>
  );
};

export default Renderer;
