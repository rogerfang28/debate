import React, { useEffect, useRef, useState, useCallback, useMemo } from "react";
import * as d3 from "d3";
import { BaseComponentProps } from "./TextComponent";
import handleEvent from "../../events/handleEvent";

// ── Types matching the backend protobuf data ──────────────────────────

interface GraphNode {
  id: string;
  text: string;
  x?: number;
  y?: number;
  type?: string;       // "claim" | "evidence" | "challenge" | "counter" | ""
  status?: string;     // "TRUE_CLAIM" | "FALSE_CLAIM" | "UNDETERMINED"
  creatorId?: number;
  creatorUsername?: string;
  isRoot?: boolean;
  isCurrent?: boolean;
}

interface GraphEdge {
  id: string;
  source: string;
  target: string;
  type?: string;       // "supports" | "challenges" | "counters" | "PARENT_CHILD" | "CHALLENGE"
}

interface DebateGraphProps extends BaseComponentProps {
  component: BaseComponentProps["component"] & {
    nodes?: GraphNode[];
    edges?: GraphEdge[];
    selectedNodeId?: string;
    selectedEdgeId?: string;
  };
  onNodeClick?: (node: GraphNode) => void;
}

// ── Color / shape mappings ────────────────────────────────────────────

const STATUS_COLORS: Record<string, { fill: string; stroke: string }> = {
  TRUE_CLAIM:     { fill: "#1e3a5f", stroke: "#34d399" },
  FALSE_CLAIM:    { fill: "#3b1a1a", stroke: "#f87171" },
  UNDETERMINED:  { fill: "#1e293b", stroke: "#9ca3af" },
};

const DEFAULT_NODE_COLOR = { fill: "#1e293b", stroke: "#6366f1" };

const EDGE_COLORS: Record<string, string> = {
  supports:   "#34d399",
  challenges: "#f97316",
  counters:   "#ef4444",
  NORMAL:     "#64748b",
  PARENT_CHILD: "#94a3b8",
  CHALLENGE:  "#f97316",
};

// ── Per-author color badge ─────────────────────────────────────────────
// Deterministic (not order-of-appearance) so a given username always gets
// the same color across reloads/debates, independent of node status color.

const AUTHOR_PALETTE = [
  "#60a5fa", "#f472b6", "#facc15", "#34d399",
  "#a78bfa", "#fb923c", "#22d3ee", "#f87171",
];

function hashString(s: string): number {
  let hash = 0;
  for (let i = 0; i < s.length; i++) {
    hash = (hash << 5) - hash + s.charCodeAt(i);
    hash |= 0;
  }
  return Math.abs(hash);
}

function colorForAuthor(username: string): string {
  if (!username) return "#6b7280";
  return AUTHOR_PALETTE[hashString(username) % AUTHOR_PALETTE.length];
}

const NODE_WIDTH = 160;
const NODE_HEIGHT = 80;
const ROOT_WIDTH = 180;
const ROOT_HEIGHT = 88;
const CHARS_PER_LINE = 20;
const MAX_LINES = 3;
const LINE_HEIGHT = 12;

// ── Text wrapping helper ──────────────────────────────────────────────

function wrapText(text: string, maxCharsPerLine: number, maxLines: number): string[] {
  // Split into words, then force-break any word longer than a line on its
  // own (e.g. a continuous run with no spaces) into fixed-size chunks so it
  // still wraps instead of overflowing the node.
  const rawWords = text.split(/\s+/).filter(Boolean);
  const words: string[] = [];
  for (const word of rawWords) {
    if (word.length <= maxCharsPerLine) {
      words.push(word);
    } else {
      for (let i = 0; i < word.length; i += maxCharsPerLine) {
        words.push(word.slice(i, i + maxCharsPerLine));
      }
    }
  }

  const lines: string[] = [];
  let current = "";
  let wordIndex = 0;

  for (; wordIndex < words.length; wordIndex++) {
    const word = words[wordIndex];
    const test = current ? `${current} ${word}` : word;
    if (test.length > maxCharsPerLine && current) {
      lines.push(current);
      current = word;
      if (lines.length === maxLines) {
        wordIndex++;
        break;
      }
    } else {
      current = test;
    }
  }
  if (lines.length < maxLines && current) {
    lines.push(current);
    wordIndex = words.length;
  }

  if (wordIndex < words.length && lines.length > 0) {
    const last = lines[lines.length - 1];
    lines[lines.length - 1] =
      last.length > maxCharsPerLine - 1 ? last.slice(0, maxCharsPerLine - 1) + "…" : last + "…";
  }

  return lines.slice(0, maxLines);
}

// ── Component ─────────────────────────────────────────────────────────

const DebateGraph: React.FC<DebateGraphProps> = ({ component, className, style, onNodeClick }) => {
  const svgRef = useRef<SVGSVGElement>(null);
  const containerRef = useRef<HTMLDivElement>(null);
  const [dims, setDims] = useState({ width: 800, height: 600 });
  const [selectedId, setSelectedId] = useState<string | null>(component.selectedNodeId || null);
  const [tooltip, setTooltip] = useState<{ text: string; x: number; y: number } | null>(null);

  const { nodes = [], edges = [] } = component;

  // ── Unique authors present, in first-seen order, for the legend ──
  const uniqueAuthors = useMemo(() => {
    const seen = new Set<string>();
    const list: string[] = [];
    for (const n of nodes) {
      if (n.creatorUsername && !seen.has(n.creatorUsername)) {
        seen.add(n.creatorUsername);
        list.push(n.creatorUsername);
      }
    }
    return list;
  }, [nodes]);

  // ── Track container size for responsive SVG ───────────────────────
  useEffect(() => {
    const el = containerRef.current;
    if (!el) return;

    const ro = new ResizeObserver((entries) => {
      for (const entry of entries) {
        const { width, height } = entry.contentRect;
        if (width > 0 && height > 0) {
          setDims({ width: Math.round(width), height: Math.round(height) });
        }
      }
    });
    ro.observe(el);
    return () => ro.disconnect();
  }, []);

  // ── Compute tree layout positions ─────────────────────────────────
  const GAP_X = 200;
  const GAP_Y = 170;

  const positionedNodes = useMemo(() => {
    if (nodes.length === 0) return [];

    const nodeMap = new Map<string, GraphNode>();
    nodes.forEach((n) => nodeMap.set(n.id, { ...n }));

    // Build children map from PARENT_CHILD edges
    const childrenMap = new Map<string, string[]>();
    const hasParent = new Set<string>();
    for (const e of edges) {
      if (e.type === "PARENT_CHILD") {
        if (!childrenMap.has(e.source)) childrenMap.set(e.source, []);
        childrenMap.get(e.source)!.push(e.target);
        hasParent.add(e.target);
      }
    }

    // Find root (node with no parent, prefer isRoot flag)
    let rootId = nodes.find((n) => n.isRoot)?.id ?? "";
    if (!rootId) {
      rootId = nodes.find((n) => !hasParent.has(n.id))?.id ?? nodes[0]?.id ?? "";
    }

    // BFS to assign levels and order
    const positioned: GraphNode[] = [];
    const visited = new Set<string>();
    let currentLevel = [rootId];

    while (currentLevel.length > 0) {
      const nextLevel: string[] = [];
      for (const id of currentLevel) {
        if (visited.has(id)) continue;
        visited.add(id);
        const node = nodeMap.get(id);
        if (node) {
          positioned.push(node);
          const children = childrenMap.get(id) ?? [];
          for (const c of children) {
            if (!visited.has(c)) nextLevel.push(c);
          }
        }
      }
      currentLevel = nextLevel;
    }

    // Nodes not connected to the root (no PARENT_CHILD path from it) are
    // dropped entirely for now, instead of floating in a row underneath.

    // Compute subtree widths
    const subtreeWidth = new Map<string, number>();
    const computeWidth = (id: string): number => {
      if (subtreeWidth.has(id)) return subtreeWidth.get(id)!;
      const children = childrenMap.get(id) ?? [];
      if (children.length === 0) {
        subtreeWidth.set(id, GAP_X);
        return GAP_X;
      }
      const total = children.reduce((sum, c) => sum + computeWidth(c), 0);
      const w = Math.max(GAP_X, total);
      subtreeWidth.set(id, w);
      return w;
    };
    computeWidth(rootId);

    // Place nodes: root at top center, children spread below
    const totalWidth = computeWidth(rootId);
    const xOffset = (dims.width - totalWidth) / 2;

    const placeNode = (id: string, x: number, depth: number) => {
      const node = nodeMap.get(id);
      if (!node) return;
      const w = subtreeWidth.get(id) ?? GAP_X;
      node.x = x + w / 2;
      node.y = 60 + depth * GAP_Y;
      const children = childrenMap.get(id) ?? [];
      let cx = x;
      for (const c of children) {
        const cw = subtreeWidth.get(c) ?? GAP_X;
        placeNode(c, cx, depth + 1);
        cx += cw;
      }
    };
    placeNode(rootId, xOffset, 0);

    return positioned;
  }, [nodes, edges, dims.width]);

  // ── Zoom & pan ────────────────────────────────────────────────────
  useEffect(() => {
    const svg = d3.select(svgRef.current);
    if (!svg.node()) return;

    const zoom = d3
      .zoom<SVGSVGElement, unknown>()
      .scaleExtent([0.2, 3])
      .on("zoom", (event) => {
        svg.select("g.graph-root").attr("transform", event.transform.toString());
      });

    svg.call(zoom);
  }, []);

  // ── Click handler — selects the claim server-side (SELECT_CLAIM), which
  // moves the "current claim" highlight to it on next render.
  const handleNodeClick = useCallback(
    (node: GraphNode, e: React.MouseEvent) => {
      setSelectedId(node.id === selectedId ? null : node.id);
      onNodeClick?.(node);
      const syntheticId = `selectClaimNode_${node.id}`;
      handleEvent(e as any, { id: syntheticId }, "onClick", syntheticId);
    },
    [selectedId, onNodeClick]
  );

  // ── No-data state ─────────────────────────────────────────────────
  if (nodes.length === 0 && edges.length === 0) {
    return (
      <div
        id={component.id}
        className={`node-graph ${className || ""}`}
        style={{
          ...style,
          background: "var(--surface-card)",
          border: "1px solid var(--border-subtle)",
          borderRadius: "var(--radius-lg)",
          padding: "var(--space-lg)",
          display: "flex",
          alignItems: "center",
          justifyContent: "center",
          minHeight: "120px",
        }}
      >
        <p style={{ color: "var(--text-muted)", fontSize: "0.875rem", margin: 0 }}>
          No graph data yet — add claims to see the debate map.
        </p>
      </div>
    );
  }

  // ── Compute positions for rendering ──────────────────────────────
  const nodeMap = new Map<string, GraphNode>();
  positionedNodes.forEach((n) => nodeMap.set(n.id, n));

  return (
    <div
      id={component.id}
      className={`node-graph ${className || ""}`}
      style={{
        ...style,
        background: "var(--surface-card)",
        border: "1px solid var(--border-subtle)",
        borderRadius: "var(--radius-lg)",
        overflow: "hidden",
        position: "relative",
      }}
      ref={containerRef}
    >
      <svg
        ref={svgRef}
        width={dims.width}
        height={dims.height}
        viewBox={`0 0 ${dims.width} ${dims.height}`}
        style={{ display: "block", width: "100%", height: "100%" }}
      >
        <defs>
          {/* Glow filter for selected / current nodes */}
          <filter id={`${component.id}-glow`} x="-50%" y="-50%" width="200%" height="200%">
            <feGaussianBlur stdDeviation="4" result="coloredBlur" />
            <feMerge>
              <feMergeNode in="coloredBlur" />
              <feMergeNode in="SourceGraphic" />
            </feMerge>
          </filter>
          {/* Arrow markers */}
          <marker
            id={`${component.id}-arrow`}
            markerWidth="10"
            markerHeight="7"
            refX="9"
            refY="3.5"
            orient="auto"
          >
            <polygon points="0 0, 10 3.5, 0 7" fill="#64748b" />
          </marker>
          <marker
            id={`${component.id}-arrow-challenge`}
            markerWidth="10"
            markerHeight="7"
            refX="9"
            refY="3.5"
            orient="auto"
          >
            <polygon points="0 0, 10 3.5, 0 7" fill="#f97316" />
          </marker>
          <marker
            id={`${component.id}-arrow-support`}
            markerWidth="10"
            markerHeight="7"
            refX="9"
            refY="3.5"
            orient="auto"
          >
            <polygon points="0 0, 10 3.5, 0 7" fill="#34d399" />
          </marker>
        </defs>

        <g className="graph-root">
          {/* ── Edges ─────────────────────────────────────────────── */}
          {edges.map((edge, i) => {
            const source = nodeMap.get(edge.source);
            const target = nodeMap.get(edge.target);
            if (!source || !target) return null;

            const sx = source.x ?? 100;
            const sy = source.y ?? 100;
            const tx = target.x ?? 200;
            const ty = target.y ?? 200;

            const isChallenge = edge.type === "CHALLENGE" || edge.type === "challenges";
            const color = isChallenge
              ? EDGE_COLORS.challenges
              : edge.type === "PARENT_CHILD" || edge.type === "supports"
                ? EDGE_COLORS.PARENT_CHILD
                : EDGE_COLORS.NORMAL;
            const dash = isChallenge ? "6,3" : "none";

            return (
              <line
                key={edge.id || i}
                x1={sx}
                y1={sy}
                x2={tx}
                y2={ty}
                stroke={color}
                strokeWidth={isChallenge ? 2.5 : 1.5}
                strokeOpacity={0.7}
                strokeDasharray={dash}
                markerEnd={`url(#${component.id}-${isChallenge ? "arrow-challenge" : "arrow-support"})`}
                style={{ transition: "x1 0.05s, y1 0.05s, x2 0.05s, y2 0.05s" }}
              />
            );
          })}

          {/* ── Nodes ─────────────────────────────────────────────── */}
          {positionedNodes.map((node) => {
            const cx = node.x ?? 100;
            const cy = node.y ?? 100;
            const w = node.isRoot ? ROOT_WIDTH : NODE_WIDTH;
            const h = node.isRoot ? ROOT_HEIGHT : NODE_HEIGHT;
            const isSelected = node.id === selectedId;
            const isCurrent = node.isCurrent;
            const colors = STATUS_COLORS[node.status || ""] || DEFAULT_NODE_COLOR;
            const isChallenge = node.type === "challenge";
            // Challenge (root) claims get an orange-tinted body and border; the
            // current/selected highlight still takes precedence when active.
            const fill = isChallenge ? "#3a2412" : colors.fill;
            const stroke = isCurrent ? "#fbbf24" : isChallenge ? "#f97316" : colors.stroke;
            const strokeWidth = isCurrent ? 4 : isSelected ? 3 : isChallenge ? 3 : 2;
            const lines = wrapText(node.text, CHARS_PER_LINE, MAX_LINES);
            // Center the whole block (sentence + creator + status) based on
            // this node's actual line count, instead of reserving fixed
            // room for MAX_LINES -- keeps everything close together
            // regardless of how long the sentence is.
            const sentenceGap = 4;
            const contentHeight = lines.length * LINE_HEIGHT + sentenceGap + 2 * LINE_HEIGHT;
            const sentenceTop = -contentHeight / 2 + LINE_HEIGHT * 0.8;
            const creatorY = sentenceTop + (lines.length - 1) * LINE_HEIGHT + sentenceGap + LINE_HEIGHT;
            const statusY = creatorY + LINE_HEIGHT;
            const statusLabel =
              node.status === "TRUE_CLAIM" ? "Status: True" :
              node.status === "FALSE_CLAIM" ? "Status: False" :
              "Status: Undetermined";
            const statusColor =
              node.status === "TRUE_CLAIM" ? "#34d399" :
              node.status === "FALSE_CLAIM" ? "#f87171" :
              "var(--text-muted)";

            return (
              <g
                key={node.id}
                className="graph-node"
                style={{ cursor: "pointer" }}
                transform={`translate(${cx},${cy})`}
                onClick={(e) => handleNodeClick(node, e)}
                onPointerEnter={(e) => {
                  const rect = containerRef.current?.getBoundingClientRect();
                  setTooltip({
                    text: node.text,
                    x: e.clientX - (rect?.left ?? 0),
                    y: e.clientY - (rect?.top ?? 0),
                  });
                }}
                onPointerLeave={() => setTooltip(null)}
              >
                {/* Glow ring */}
                {(isSelected || isCurrent) && (
                  <rect
                    x={-w / 2 - 8}
                    y={-h / 2 - 8}
                    width={w + 16}
                    height={h + 16}
                    rx={14}
                    ry={14}
                    fill="none"
                    stroke={isCurrent ? "#fbbf24" : colors.stroke}
                    strokeWidth="2"
                    strokeOpacity="0.4"
                    filter={`url(#${component.id}-glow)`}
                  />
                )}
                {/* Node body */}
                <rect
                  x={-w / 2}
                  y={-h / 2}
                  width={w}
                  height={h}
                  rx={10}
                  ry={10}
                  fill={fill}
                  stroke={stroke}
                  strokeWidth={strokeWidth}
                />
                {/* Claim sentence (wrapped, up to MAX_LINES), top-anchored */}
                <text
                  textAnchor="middle"
                  fill="var(--text-primary)"
                  fontSize="10"
                  fontWeight="500"
                  style={{ pointerEvents: "none", userSelect: "none" }}
                >
                  {lines.map((line, i) => (
                    <tspan key={i} x={0} y={sentenceTop + i * LINE_HEIGHT}>
                      {line}
                    </tspan>
                  ))}
                </text>
                {/* Author badge — colored dot + initial, top-left corner.
                    Separate visual channel from the status color (node fill/
                    border) so the two don't compete for the same signal. */}
                {node.creatorUsername && (
                  <g style={{ pointerEvents: "none" }}>
                    <circle
                      cx={-w / 2 + 12}
                      cy={-h / 2 + 12}
                      r={7}
                      fill={colorForAuthor(node.creatorUsername)}
                      stroke="var(--surface-card)"
                      strokeWidth={1.5}
                    />
                    <text
                      x={-w / 2 + 12}
                      y={-h / 2 + 12}
                      textAnchor="middle"
                      dominantBaseline="central"
                      fontSize="8"
                      fontWeight="700"
                      fill="#0f1117"
                      style={{ userSelect: "none" }}
                    >
                      {node.creatorUsername.charAt(0).toUpperCase()}
                    </text>
                  </g>
                )}
                {/* Created by */}
                <text
                  textAnchor="middle"
                  y={creatorY}
                  fill="var(--text-muted)"
                  fontSize="8"
                  style={{ pointerEvents: "none" }}
                >
                  Created by: {node.creatorUsername || `#${node.id}`}
                </text>
                {/* Status */}
                <text
                  textAnchor="middle"
                  y={statusY}
                  fill={statusColor}
                  fontSize="8"
                  fontWeight="600"
                  style={{ pointerEvents: "none" }}
                >
                  {statusLabel}
                </text>
              </g>
            );
          })}
        </g>
      </svg>

      {/* Tooltip */}
      {tooltip && (
        <div
          className="graph-tooltip"
          style={{
            position: "absolute",
            left: tooltip.x + 12,
            top: tooltip.y - 10,
            background: "var(--surface-elevated)",
            border: "1px solid var(--border-default)",
            borderRadius: "var(--radius-md)",
            padding: "0.5rem 0.75rem",
            fontSize: "0.8125rem",
            color: "var(--text-primary)",
            maxWidth: "280px",
            pointerEvents: "none",
            zIndex: 100,
            boxShadow: "var(--shadow-lg)",
          }}
        >
          {tooltip.text}
        </div>
      )}

      {/* Controls */}
      <div
        style={{
          position: "absolute",
          bottom: "0.75rem",
          right: "0.75rem",
          display: "flex",
          gap: "0.375rem",
        }}
      >
        <button
          onClick={() => {
            const svg = d3.select(svgRef.current);
            svg
              .transition()
              .duration(300)
              .call(
                d3.zoom<SVGSVGElement, unknown>().transform,
                d3.zoomIdentity.translate(0, 0).scale(1)
              );
          }}
          className="graph-zoom-btn"
          title="Reset zoom"
          style={{
            width: "28px",
            height: "28px",
            display: "flex",
            alignItems: "center",
            justifyContent: "center",
            background: "var(--surface-elevated)",
            border: "1px solid var(--border-default)",
            borderRadius: "var(--radius-sm)",
            cursor: "pointer",
            color: "var(--text-secondary)",
            fontSize: "0.8125rem",
          }}
        >
          ⟲
        </button>
        <button
          onClick={() => {
            const svg = d3.select(svgRef.current);
            const root = svg.select<SVGGElement>("g.graph-root");
            if (!root.node()) return;
            const bounds = (root.node() as SVGGElement).getBBox();
            const padding = 40;
            const scale = Math.min(
              (dims.width - padding * 2) / (bounds.width || 1),
              (dims.height - padding * 2) / (bounds.height || 1),
              2
            );
            const tx = dims.width / 2 - ((bounds.x || 0) + (bounds.width || 0) / 2) * scale;
            const ty = dims.height / 2 - ((bounds.y || 0) + (bounds.height || 0) / 2) * scale;
            svg
              .transition()
              .duration(400)
              .call(
                d3.zoom<SVGSVGElement, unknown>().transform,
                d3.zoomIdentity.translate(tx, ty).scale(scale)
              );
          }}
          className="graph-zoom-btn"
          title="Fit to view"
          style={{
            width: "28px",
            height: "28px",
            display: "flex",
            alignItems: "center",
            justifyContent: "center",
            background: "var(--surface-elevated)",
            border: "1px solid var(--border-default)",
            borderRadius: "var(--radius-sm)",
            cursor: "pointer",
            color: "var(--text-secondary)",
            fontSize: "0.8125rem",
          }}
        >
          ⊞
        </button>
      </div>

      {/* Legend */}
      <div
        style={{
          position: "absolute",
          top: "0.75rem",
          left: "0.75rem",
          display: "flex",
          flexWrap: "wrap",
          gap: "0.5rem",
          fontSize: "0.6875rem",
          color: "var(--text-muted)",
          background: "rgba(15, 17, 23, 0.8)",
          padding: "0.375rem 0.5rem",
          borderRadius: "var(--radius-sm)",
          border: "1px solid var(--border-subtle)",
        }}
      >
        <span style={{ display: "flex", alignItems: "center", gap: "0.25rem" }}>
          <span style={{ width: 8, height: 8, borderRadius: "50%", background: "#34d399", display: "inline-block" }} />
          True
        </span>
        <span style={{ display: "flex", alignItems: "center", gap: "0.25rem" }}>
          <span style={{ width: 8, height: 8, borderRadius: "50%", background: "#f87171", display: "inline-block" }} />
          False
        </span>
        <span style={{ display: "flex", alignItems: "center", gap: "0.25rem" }}>
          <span style={{ width: 8, height: 8, borderRadius: "50%", background: "#9ca3af", display: "inline-block" }} />
          Undetermined
        </span>
        <span style={{ display: "flex", alignItems: "center", gap: "0.25rem" }}>
          <span style={{ width: 12, height: 2, background: "#f97316", display: "inline-block" }} />
          Challenge
        </span>
        {uniqueAuthors.length > 0 && (
          <span
            style={{
              width: 1,
              alignSelf: "stretch",
              background: "var(--border-subtle)",
              margin: "0 0.125rem",
            }}
          />
        )}
        {uniqueAuthors.map((author) => (
          <span key={author} style={{ display: "flex", alignItems: "center", gap: "0.25rem" }}>
            <span
              style={{
                width: 8,
                height: 8,
                borderRadius: "50%",
                background: colorForAuthor(author),
                display: "inline-block",
              }}
            />
            {author}
          </span>
        ))}
      </div>
    </div>
  );
};

export default DebateGraph;
