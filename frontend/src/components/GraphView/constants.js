// src/features/graph/constants.js
// -----------------------------------------------------------------------------
// Shared design tokens for the graph feature. Keep *all* visual constants here so
// themes or colours can be tweaked without hunting across files.
// -----------------------------------------------------------------------------
export const COLORS = {
  DEFAULT_NODE: "#8b5cf6",
  SELECTED: "#6366f1",
  HOVERED: "#f59e0b",
  CHALLENGER: "#eab308",
  RESPONDER: "#ef4444",
  SHADOW: {
    SELECTED: "#6366f1",
    HOVERED: "#f59e0b",
    CHALLENGER: "#eab308",
    RESPONDER: "#ef4444",
  },
  BORDER: {
    DEFAULT: "rgba(255,255,255,0.3)",
    SELECTED: "#a5b4fc",
    HOVERED: "#fbbf24",
    CHALLENGER: "#fde047",
    RESPONDER: "#fca5a5",
  },
};

export const STYLES = {
  NODE_SIZE: 4,
  SHADOW_BLUR: 15,
  LABEL_BACKGROUND: "rgba(0, 0, 0, 0.7)",
  LABEL_TEXT: "rgba(255, 255, 255, 0.95)",
  CONTAINER: {
    position: "fixed",
    top: 0,
    left: 0,
    width: "100vw",
    height: "100vh",
    background: "linear-gradient(135deg, #0f172a 0%, #1e293b 100%)",
    backgroundImage: `
      radial-gradient(circle at 25% 25%, rgba(99, 102, 241, 0.1) 0%, transparent 50%),
      radial-gradient(circle at 75% 75%, rgba(139, 92, 246, 0.1) 0%, transparent 50%)
    `,
  },
};