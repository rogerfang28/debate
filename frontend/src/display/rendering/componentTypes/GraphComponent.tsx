import React, { useEffect, useRef, useState, useCallback } from "react";
import * as d3 from "d3";
import { BaseComponentProps } from "./TextComponent";

// ── Types matching the backend protobuf data ──────────────────────────

interface GraphNode {
  id: string;
  text: string;
  x?: number;
  y?: number;
  type?: string;       // "claim" | "evidence" | "challenge" | "counter" | ""
  status?: string;     // "TRUE_CLAIM" | "FALSE_CLAIM" | "UNDETERMINED"
  creatorId?: number;
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

const NODE_RADIUS = 36;
const ROOT_RADIUS = 44;
const LABEL_MAX_LEN = 20;

// ── Truncate helper ───────────────────────────────────────────────────

function truncate(text: string, max: number): string {
  if (text.length <= max) return text;
  return text.slice(0, max - 1) + "…";
}

// ── Component ─────────────────────────────────────────────────────────

const DebateGraph: React.FC<DebateGraphProps> = ({ component, className, style, onNodeClick }) => {
  const svgRef = useRef<SVGSVGElement>(null);
  const containerRef = useRef<HTMLDivElement>(null);
  const [dims, setDims] = useState({ width: 800, height: 600 });
  const [selectedId, setSelectedId] = useState<string | null>(component.selectedNodeId || null);
  const [tooltip, setTooltip] = useState<{ text: string; x: number; y: number } | null>(null);

  const { nodes = [], edges = [] } = component;

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

  // ── Build the D3 simulation ──────────────────────────────────────
  const simulationRef = useRef<d3.Simulation<GraphNode, undefined> | null>(null);

  useEffect(() => {
    if (nodes.length === 0) return;

    // Clone nodes so D3 mutates its own copies
    const simNodes: GraphNode[] = nodes.map((n) => ({
      ...n,
      x: n.x ?? dims.width / 2 + (Math.random() - 0.5) * 200,
      y: n.y ?? dims.height / 2 + (Math.random() - 0.5) * 200,
    }));

    // Resolve edge source/target from string IDs
    const simLinks: d3.SimulationLinkDatum<GraphNode>[] = edges
      .map((e) => ({
        source: e.source,
        target: e.target,
        ...e,
      }))
      .filter((l) => {
        const s = typeof l.source === "string" ? simNodes.find((n) => n.id === l.source) : undefined;
        const t = typeof l.target === "string" ? simNodes.find((n) => n.id === l.target) : undefined;
        return s && t;
      });

    const sim = d3
      .forceSimulation<GraphNode>(simNodes)
      .force(
        "link",
        d3
          .forceLink<GraphNode, d3.SimulationLinkDatum<GraphNode>>(simLinks)
          .id((d) => d.id)
          .distance(130)
          .strength(0.6)
      )
      .force("charge", d3.forceManyBody<GraphNode>().strength(-500))
      .force("center", d3.forceCenter(dims.width / 2, dims.height / 2).strength(0.05))
      .force(
        "collision",
        d3.forceCollide<GraphNode>().radius((d) => (d.isRoot ? ROOT_RADIUS : NODE_RADIUS) + 10)
      );

    simulationRef.current = sim;

    return () => {
      sim.stop();
    };
  }, [nodes.length, edges.length, dims.width, dims.height]); // eslint-disable-line react-hooks/exhaustive-deps

  // ── Render simulation tick → update SVG ──────────────────────────
  const [tick, setTick] = useState(0);

  useEffect(() => {
    const sim = simulationRef.current;
    if (!sim) return;

    sim.alpha(1).restart();
    sim.on("tick", () => setTick((t) => t + 1));

    return () => {
      sim.off("tick");
    };
  }, [nodes.length, edges.length]); // eslint-disable-line react-hooks/exhaustive-deps

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

  // ── Node drag ─────────────────────────────────────────────────────
  const dragStarted = useCallback(
    (event: React.PointerEvent<SVGGElement>, d: GraphNode) => {
      const sim = simulationRef.current;
      if (!sim) return;
      sim.alphaTarget(0.3).restart();
      (d as any).fx = d.x;
      (d as any).fy = d.y;
    },
    []
  );

  const dragged = useCallback(
    (event: React.PointerEvent<SVGGElement>, d: GraphNode) => {
      (d as any).fx = event.clientX;
      (d as any).fy = event.clientY;
    },
    []
  );

  const dragEnded = useCallback(
    (event: React.PointerEvent<SVGGElement>, d: GraphNode) => {
      const sim = simulationRef.current;
      if (!sim) return;
      sim.alphaTarget(0);
      (d as any).fx = null;
      (d as any).fy = null;
    },
    []
  );

  // ── Click handler ─────────────────────────────────────────────────
  const handleNodeClick = useCallback(
    (node: GraphNode) => {
      setSelectedId(node.id === selectedId ? null : node.id);
      onNodeClick?.(node);
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
  nodes.forEach((n) => nodeMap.set(n.id, n));

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

            // Use simulation positions when available (tick forces re-render)
            void tick; // dependency marker — don't remove
            const sx = (source as any).x ?? source.x ?? 100;
            const sy = (source as any).y ?? source.y ?? 100;
            const tx = (target as any).x ?? target.x ?? 200;
            const ty = (target as any).y ?? target.y ?? 200;

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
          {nodes.map((node) => {
            // tick forces re-render on simulation step
            void tick;
            const cx = (node as any).x ?? node.x ?? 100;
            const cy = (node as any).y ?? node.y ?? 100;
            const r = node.isRoot ? ROOT_RADIUS : NODE_RADIUS;
            const isSelected = node.id === selectedId;
            const isCurrent = node.isCurrent;
            const colors = STATUS_COLORS[node.status || ""] || DEFAULT_NODE_COLOR;
            const fill = isCurrent ? colors.fill : colors.fill;
            const stroke = isCurrent ? "#fbbf24" : colors.stroke;
            const strokeWidth = isCurrent ? 4 : isSelected ? 3 : 2;

            return (
              <g
                key={node.id}
                className="graph-node"
                style={{ cursor: "pointer" }}
                transform={`translate(${cx},${cy})`}
                onClick={() => handleNodeClick(node)}
                onPointerDown={(e) => dragStarted(e, node)}
                onPointerMove={(e) => dragged(e, node)}
                onPointerUp={(e) => dragEnded(e, node)}
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
                  <circle
                    r={r + 8}
                    fill="none"
                    stroke={isCurrent ? "#fbbf24" : colors.stroke}
                    strokeWidth="2"
                    strokeOpacity="0.4"
                    filter={`url(#${component.id}-glow)`}
                  />
                )}
                {/* Node body */}
                <circle
                  r={r}
                  fill={fill}
                  stroke={stroke}
                  strokeWidth={strokeWidth}
                />
                {/* Status dot */}
                {!node.status || node.status === "UNDETERMINED" ? null : (
                  <circle
                    cx={r - 6}
                    cy={-r + 6}
                    r={5}
                    fill={node.status === "TRUE_CLAIM" ? "#34d399" : "#f87171"}
                  />
                )}
                {/* Label */}
                <text
                  textAnchor="middle"
                  dy="0.35em"
                  fill="var(--text-primary)"
                  fontSize="10"
                  fontWeight="500"
                  style={{ pointerEvents: "none", userSelect: "none" }}
                >
                  {truncate(node.text, LABEL_MAX_LEN)}
                </text>
                {/* ID badge */}
                <text
                  textAnchor="middle"
                  dy="1.5em"
                  fill="var(--text-muted)"
                  fontSize="8"
                  style={{ pointerEvents: "none" }}
                >
                  #{node.id}
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
            position: "fixed",
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
      </div>
    </div>
  );
};

export default DebateGraph;
