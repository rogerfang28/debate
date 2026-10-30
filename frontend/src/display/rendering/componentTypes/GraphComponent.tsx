import React, { useEffect, useLayoutEffect, useRef, useState, useCallback, useMemo } from "react";
import * as d3 from "d3";
import { BaseComponentProps } from "./TextComponent";
import handleEvent from "../../events/handleEvent";
import type { GraphNode, GraphEdge } from "./graphLayout";
import {
  layoutDebate,
  isSupportEdge,
  isChallengeEdge,
  JUNCTION_HEIGHT,
  JUNCTION_DOT_RADIUS,
  JUNCTION_CHAR_WIDTH,
  JUNCTION_PADDING_X,
} from "./graphLayout";
import { focusOf, navigationFor, placeConnectionCards, lineKey } from "./connectionCards";
import type { CardTarget } from "./connectionCards";

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

// Theme tokens (index.css). Every node is the raised surface; its border
// says where it stands: green held true, red held false, the accent for the
// main claim, a quiet line otherwise. A CHALLENGE claim is orange, body and
// border, so "this is a challenge" never looks like "this is held false".
// The fill is opaque on purpose -- nodes are drawn over the line ends and
// hide them.
const STATUS_COLORS: Record<string, { fill: string; stroke: string }> = {
  TRUE_CLAIM:     { fill: "var(--panel-2)", stroke: "var(--settled)" },
  FALSE_CLAIM:    { fill: "var(--panel-2)", stroke: "var(--open)" },
  UNDETERMINED:  { fill: "var(--panel-2)", stroke: "var(--line-strong)" },
};

const DEFAULT_NODE_COLOR = { fill: "var(--panel-2)", stroke: "var(--accent)" };
const ROOT_STROKE = "var(--accent)";
const CURRENT_STROKE = "var(--warn)";

// The colour of a claim's status, as the node's "Status:" line and the
// legend show it: green true, red false, grey undetermined. The connection
// cards wear this, so a card says at a glance where its claim stands.
function statusColorOf(node: GraphNode): string {
  return node.status === "TRUE_CLAIM" ? "var(--settled)" :
    node.status === "FALSE_CLAIM" ? "var(--open)" :
    "var(--text-3)";
}

// A node's border when nothing is highlighting it: a challenge is orange
// whatever its status, the main claim wears the accent until it is settled,
// and anything else shows its status.
function restingStroke(node: GraphNode): string {
  const colors = STATUS_COLORS[node.status || ""] || DEFAULT_NODE_COLOR;
  if (node.type === "challenge") return "var(--challenge)";
  if (node.isRoot && (node.status || "UNDETERMINED") === "UNDETERMINED") return ROOT_STROKE;
  return colors.stroke;
}

const EDGE_COLORS: Record<string, string> = {
  challenges: "var(--challenge)",
  counters:   "var(--challenge)",
  NORMAL:     "var(--text-3)",
  PARENT_CHILD: "var(--text-3)",
  CHALLENGE:  "var(--challenge)",
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
  if (!username) return "var(--text-3)";
  return AUTHOR_PALETTE[hashString(username) % AUTHOR_PALETTE.length];
}

const NODE_WIDTH = 160;
const NODE_HEIGHT = 80;

// ── The camera, remembered ────────────────────────────────────────────
//
// Every click re-renders the page from a fresh server tree, and a remount
// (a tab switch, Brief and back) starts the graph from scratch. The reader's
// pan and zoom are theirs, so they are kept here, outside React, per graph:
// keyed by the GRAPH component's id and the root claim's id, which together
// name one debate's map (claim ids are unique across debates).
//
// A stored camera is reused only while the set of claims drawn is the one it
// was taken over. When claims are added or removed the graph refits once, and
// that becomes the stored camera. A selection, a status change or a poll
// changes no node ids, so it never moves the camera.
type Camera = { nodeSet: string; transform: d3.ZoomTransform };
const cameras = new Map<string, Camera>();

// ── Fly-to ────────────────────────────────────────────────────────────
//
// Selecting a claim flies the camera to it. The claim the camera last went to
// is remembered per graph, beside the camera, so the server's echo of a click
// (the same selection, re-rendered) does not fly again, while a selection made
// elsewhere -- the Brief's "Open in debate", the text view -- does, including
// across a remount.
const flownTo = new Map<string, string>();
const FLY_MS = 450;
// Fly-to is a mode: on, a click flies to the claim, lights its connections
// and draws the edge cards; off, a click only selects, as the graph always
// did. The choice is the viewer's, kept in their browser.
const FLY_STORAGE_KEY = "debate.graphFly";
// The key (what the colours and the dashed line mean, and whose colour is
// whose) is folded behind a button so it never covers the graph; the reader
// opens it when they need it, and their browser remembers which way it was.
const KEY_STORAGE_KEY = "debate.graphKey";
const readKeyOpen = (): boolean => {
  try {
    return localStorage.getItem(KEY_STORAGE_KEY) === "open";
  } catch {
    return false;
  }
};
const readFlyMode = (): boolean => {
  try {
    return localStorage.getItem(FLY_STORAGE_KEY) !== "off";
  } catch {
    return true;
  }
};
// On-screen scale a flight lands at, converted into the transform's units
// the same way the opening view converts MIN_READABLE_SCALE. 1 is above the
// "full" semantic-zoom tier (0.85), so the node shows its sentence, author and
// status rather than a counter-scaled two-line gist. Never zooms OUT: a reader
// already closer than this stays where they are.
const FLY_READABLE_SCALE = 1;
const easeOutCubic = (t: number) => 1 - Math.pow(1 - t, 3);

// The layout is computed at one fixed width, not the pane's. Positions then
// depend on the debate alone, so a pane that changes width (the chat folded
// or opened) keeps every claim where the camera expects it; the pane just
// shows more or less around it.
const LAYOUT_WIDTH = 800;


// ══════════════════════════════════════════════════════════════════════
// EXPERIMENT: SEMANTIC ZOOM  ── everything for it is fenced by these
// banners, so it can be removed by deleting the fenced blocks and the two
// call sites they mention. Nothing outside them changed behaviour.
//
// The problem: at the zoom needed to fit a real debate, a node is ~55x28
// css px and its text renders at ~3.8px. No font size fixes that -- a box
// that small cannot hold a sentence. So instead of shrinking text, change
// WHAT a node shows at each scale.
//
// Safe to drop text because reading already lives elsewhere: the side
// panel shows the selected claim in full, and hovering any node shows a
// tooltip. Zoomed out the graph becomes a map you navigate; zoomed in it
// becomes a document you read.
//
// Toggle lives in the graph controls (⌸). Default ON so the difference is
// visible; flip SEMANTIC_ZOOM_DEFAULT to false to ship the old look.
// ══════════════════════════════════════════════════════════════════════
const SEMANTIC_ZOOM_DEFAULT = true;

type DetailTier = "map" | "compact" | "full";

// Thresholds are in FINAL on-screen scale, so they mean the same thing
// regardless of viewBox size.
function tierForScale(scale: number): DetailTier {
  if (scale < 0.5) return "map";
  if (scale < 0.85) return "compact";
  return "full";
}
// ══════════════════════ END EXPERIMENT (constants) ════════════════════

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

  // ═══════════ EXPERIMENT: SEMANTIC ZOOM (state) ═══════════
  const [semanticZoom, setSemanticZoom] = useState(SEMANTIC_ZOOM_DEFAULT);
  const [flyMode, setFlyMode] = useState(readFlyMode);
  const [keyOpen, setKeyOpen] = useState(readKeyOpen);
  const toggleKey = useCallback(() => {
    setKeyOpen((v) => {
      try {
        localStorage.setItem(KEY_STORAGE_KEY, v ? "closed" : "open");
      } catch {
        // storage blocked: the choice lasts for this page
      }
      return !v;
    });
  }, []);
  const flyModeRef = useRef(flyMode);
  flyModeRef.current = flyMode;
  const toggleFlyMode = useCallback(() => {
    setFlyMode((v) => {
      try {
        localStorage.setItem(FLY_STORAGE_KEY, v ? "off" : "on");
      } catch {
        // storage blocked: the choice lasts for this page
      }
      return !v;
    });
  }, []);
  // Only the TIER is kept in state, not the raw scale: re-rendering 22 nodes
  // on every wheel tick would be wasteful, and nothing here cares about the
  // exact number -- only which of three bands it falls in.
  const [detailTier, setDetailTier] = useState<DetailTier>("full");
  // The scale itself is needed to counter-scale text. Quantised so a wheel
  // gesture re-renders a handful of times rather than on every tick.
  const [zoomScale, setZoomScale] = useState(1);
  // ═══════════ END EXPERIMENT (state) ═══════════
  const [selectedId, setSelectedId] = useState<string | null>(component.selectedNodeId || null);
  // The server is the authority on what is selected. Local state exists only
  // so a click echoes before the round trip lands; without this sync a
  // selection made OUTSIDE the graph -- a claim clicked in the paragraph
  // tree -- left the graph believing the old pill was still selected, and
  // the claim's highlight suppressed along with it.
  useEffect(() => {
    setSelectedId(component.selectedNodeId || null);
  }, [component.selectedNodeId]);
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
          const next = { width: Math.round(width), height: Math.round(height) };
          // Same size, same object: no re-render, nothing downstream re-runs.
          setDims((prev) => (prev.width === next.width && prev.height === next.height ? prev : next));
        }
      }
    });
    ro.observe(el);
    return () => ro.disconnect();
  }, []);

  // ── Layout: claims and relations together (see layoutDebate) ──────
  const { positionedNodes, relationGroups } = useMemo(
    () => layoutDebate(nodes, edges, LAYOUT_WIDTH),
    [nodes, edges]
  );

  // Which camera this is, and the set of claims it was taken over.
  const rootClaimId = (nodes.find((n) => n.isRoot) ?? nodes[0])?.id ?? "";
  const cameraKey = `${component.id ?? "graph"}:${rootClaimId}`;
  const nodeSet = useMemo(
    () => positionedNodes.map((n) => n.id).sort().join(","),
    [positionedNodes]
  );
  // Read by the zoom handler, which is attached once.
  const cameraKeyRef = useRef(cameraKey);
  const nodeSetRef = useRef(nodeSet);
  const hasGraph = nodes.length > 0 || edges.length > 0;

  // ── Zoom & pan ────────────────────────────────────────────────────
  //
  // The behaviour is kept in a ref so every caller drives the SAME one.
  // Previously the fit and reset buttons each constructed a throwaway
  // d3.zoom() and called .transform on it, which moves the picture but leaves
  // the ATTACHED behaviour still holding the old transform -- so the next drag
  // or wheel snapped back to wherever the graph used to be.
  const zoomRef = useRef<d3.ZoomBehavior<SVGSVGElement, unknown> | null>(null);
  // The flight in progress: its frame and where it lands. While one runs,
  // each frame's "end" stores the LANDING transform, so a remount mid-flight
  // comes back where the flight was going rather than halfway there.
  const flightRef = useRef<{ raf: number; target: d3.ZoomTransform } | null>(null);
  // Repositions the connection cards against the live camera. Called on every
  // zoom event (so every flight frame too) and after every render.
  const drawCardsRef = useRef<() => void>(() => {});

  const cancelFlight = useCallback(() => {
    if (flightRef.current) cancelAnimationFrame(flightRef.current.raf);
    flightRef.current = null;
  }, []);
  useEffect(() => cancelFlight, [cancelFlight]);

  useEffect(() => {
    const svg = d3.select(svgRef.current);
    if (!svg.node()) return;

    const zoom = d3
      .zoom<SVGSVGElement, unknown>()
      // The floor used to be 0.2, which is not far enough out to fit a real
      // debate: a 22-claim tree spans ~2600px in a ~460px pane and needs about
      // 0.15. Fit-to-view computed a scale the zoom behaviour then refused to
      // hold, so the graph sprang back on the next interaction.
      .scaleExtent([0.02, 3])
      // The reader's own drag or wheel takes the camera back from a flight.
      .on("start", (event: { sourceEvent?: Event | null }) => {
        if (event.sourceEvent) cancelFlight();
      })
      .on("zoom", (event) => {
        svg.select("g.graph-root").attr("transform", event.transform.toString());
        drawCardsRef.current();
        // ═══ EXPERIMENT: SEMANTIC ZOOM ─ report the tier, not the scale ═══
        // Multiplied by the viewBox-to-css ratio so the thresholds are in real
        // on-screen scale rather than viewBox units.
        const el = svgRef.current;
        const vb = el?.viewBox?.baseVal;
        const ratio = vb && vb.width ? el!.getBoundingClientRect().width / vb.width : 1;
        const effective = event.transform.k * ratio;
        const next = tierForScale(effective);
        setDetailTier((prev) => (prev === next ? prev : next));
        // 2% steps. Quantising at all is what keeps a drag from re-rendering
        // every node on every frame; quantising COARSELY, though, is a lie at
        // low zoom -- 5% steps rounded 0.118 down to 0.10, a 15% error, which
        // was enough to push a legible label under the drop threshold.
        const quantised = Math.round(effective * 50) / 50;
        setZoomScale((prev) => (prev === quantised ? prev : quantised));
        // ═══ END EXPERIMENT ═══
      })
      // Every pan or zoom that finishes -- the reader's, or the Fit and
      // Reset buttons' -- is the camera to come back to.
      .on("end", (event: { transform: d3.ZoomTransform }) => {
        if (!nodeSetRef.current) return;
        const transform = flightRef.current ? flightRef.current.target : event.transform;
        cameras.set(cameraKeyRef.current, { nodeSet: nodeSetRef.current, transform });
      });

    zoomRef.current = zoom;
    svg.call(zoom);
    // Attached once the svg exists: a graph that first arrives empty has none.
  }, [hasGraph]);

  // Fit the whole graph into the pane. Shared by the button and the auto-fit
  // below so there is one definition of "fitted".
  // Below this on-screen scale a node is a few pixels wide and its label is
  // dropped entirely, so the graph shows shapes and no words. Measured: a
  // 22-claim debate in a 314px pane fits at 0.10, which renders nothing
  // readable at all.
  // ON-SCREEN scale, not transform scale. Those are different numbers: the
  // SVG's viewBox is wider than the pane it renders into, so a transform of
  // 0.35 came out as 0.153 on screen -- one truncated word per node. Measured
  // on a 22-claim debate: 0.15 gives "Fauci...", 0.8 gives "Fauci's May 2021
  // testimony that NIH...". 0.6 is the compromise -- several words per node
  // with a useful number of nodes still in view.
  const MIN_READABLE_SCALE = 0.6;

  // Opening view. Deliberately NOT fit-to-view.
  //
  // Fitting everything is the right behaviour for the Fit button, where the
  // user asked to see the whole shape. It is the wrong behaviour on arrival: a
  // large debate fits only by shrinking past the point where any label can be
  // drawn, so the first thing a reader sees is twenty-two blank boxes. Reading
  // six claims and panning beats seeing twenty-two illegibly.
  //
  // So this fits if the result would be readable, and otherwise opens at a
  // readable scale anchored on the root, which is where the argument starts.
  const openReadable = useCallback(() => {
    const svg = d3.select(svgRef.current);
    const zoom = zoomRef.current;
    const rootEl = svg.select<SVGGElement>("g.graph-root").node();
    if (!svg.node() || !zoom || !rootEl) return;

    const bounds = rootEl.getBBox();
    if (!bounds.width || !bounds.height) return;

    const padding = 40;
    const fitScale = Math.min(
      (dims.width - padding * 2) / bounds.width,
      (dims.height - padding * 2) / bounds.height,
      1.5
    );

    const elForFit = svgRef.current;
    const vbForFit = elForFit?.viewBox?.baseVal;
    const fitRatio =
      vbForFit && vbForFit.width ? elForFit!.getBoundingClientRect().width / vbForFit.width : 1;
    if (fitScale * fitRatio >= MIN_READABLE_SCALE) {
      fitToViewRef.current(0);
      return;
    }

    // Too big to fit legibly: anchor on the root claim instead. Centring the
    // whole bounding box would put the middle of a wide tree on screen, which
    // is rarely where the argument begins.
    //
    // Converted out of on-screen units into the transform's own, because the
    // two differ by however much the viewBox is scaled into the pane.
    const el = svgRef.current;
    const vb = el?.viewBox?.baseVal;
    const ratio = vb && vb.width ? el!.getBoundingClientRect().width / vb.width : 1;
    const scale = ratio > 0 ? MIN_READABLE_SCALE / ratio : MIN_READABLE_SCALE;
    const root = positionedNodes.find((n) => n.isRoot) ?? positionedNodes[0];
    if (!root) return;
    const tx = dims.width / 2 - (root.x ?? 0) * scale;
    const ty = dims.height / 3 - (root.y ?? 0) * scale;
    zoom.transform(svg, d3.zoomIdentity.translate(tx, ty).scale(scale));
  }, [dims.width, dims.height, positionedNodes]);

  const fitToView = useCallback((duration = 400) => {
    const svg = d3.select(svgRef.current);
    const zoom = zoomRef.current;
    const rootNode = svg.select<SVGGElement>("g.graph-root").node();
    if (!svg.node() || !zoom || !rootNode) return;
    cancelFlight();

    // Measure in the SVG's own coordinate space. The element is rendered
    // responsively -- its viewBox is dims.width x dims.height regardless of how
    // many CSS pixels it occupies -- so the fit must be computed against the
    // viewBox, not against getBoundingClientRect.
    const bounds = rootNode.getBBox();
    if (!bounds.width || !bounds.height) return;

    const padding = 40;
    const scale = Math.min(
      (dims.width - padding * 2) / bounds.width,
      (dims.height - padding * 2) / bounds.height,
      1.5 // never zoom IN past readable on a small debate
    );
    const tx = dims.width / 2 - (bounds.x + bounds.width / 2) * scale;
    const ty = dims.height / 2 - (bounds.y + bounds.height / 2) * scale;
    const target = d3.zoomIdentity.translate(tx, ty).scale(scale);

    // Applied directly rather than through a transition.
    // svg.transition().call(zoom.transform, …) silently did nothing here, so
    // both buttons appeared dead. A transform that lands is worth more than one
    // that animates, and this is also what makes the auto-fit below safe to run
    // during the first paint.
    if (duration > 0) {
      svg.transition().duration(duration).call(zoom.transform, target);
    }
    zoom.transform(svg, target);
  }, [dims.width, dims.height, cancelFlight]);

  // ── Fly to a claim ────────────────────────────────────────────────
  //
  // Animated by hand, one zoom.transform per frame, rather than through a d3
  // transition (see fitToView on those). Each frame is a real zoom event, so
  // the semantic-zoom tier and the off-screen cards follow the flight. The
  // centre and the scale are eased separately, so the claim travels in a
  // straight line to the middle of the pane while the zoom settles.
  const nodeById = useMemo(
    () => new Map(positionedNodes.map((n) => [n.id, n] as const)),
    [positionedNodes]
  );
  // A pill (relation) is a place on the map too: it can be selected, flown
  // to and have its connections lit, exactly like a claim.
  const groupByKey = useMemo(
    () => new Map(relationGroups.map((g) => [g.key, g] as const)),
    [relationGroups]
  );
  const positionOf = useCallback(
    (id: string): { x: number; y: number } | undefined => {
      const n = nodeById.get(id);
      if (n) return { x: n.x ?? 0, y: n.y ?? 0 };
      const g = groupByKey.get(id);
      return g ? { x: g.x, y: g.y } : undefined;
    },
    [nodeById, groupByKey]
  );
  const flyTo = useCallback(
    (id: string) => {
      const el = svgRef.current;
      const zoom = zoomRef.current;
      const node = positionOf(id);
      if (!el || !zoom || !node || !flyModeRef.current) return;
      const svg = d3.select(el);
      cancelFlight();
      svg.interrupt(); // a Fit or Reset transition still running would fight it

      const vb = el.viewBox?.baseVal;
      const ratio = vb && vb.width ? el.getBoundingClientRect().width / vb.width : 1;
      const readable = ratio > 0 ? FLY_READABLE_SCALE / ratio : FLY_READABLE_SCALE;
      const from = d3.zoomTransform(el);
      const k0 = from.k;
      const k1 = Math.min(3, Math.max(k0, readable));
      const w = dims.width, h = dims.height;
      const [c0x, c0y] = from.invert([w / 2, h / 2]);
      const c1x = node.x ?? 0, c1y = node.y ?? 0;
      const at = (cx: number, cy: number, k: number) =>
        d3.zoomIdentity.translate(w / 2 - cx * k, h / 2 - cy * k).scale(k);
      const target = at(c1x, c1y, k1);

      // A hidden page gets (almost) no animation frames; nobody would see the
      // flight anyway, so land at once instead of hanging halfway.
      if (document.visibilityState === "hidden") {
        zoom.transform(svg, target);
        return;
      }

      const t0 = performance.now();
      const step = (now: number) => {
        // A frame's timestamp can predate t0 (it is the frame's start).
        const t = Math.min(1, Math.max(0, (now - t0) / FLY_MS));
        const e = easeOutCubic(t);
        if (t >= 1) {
          // Cleared first, so this last frame's "end" stores the camera as-is.
          flightRef.current = null;
          zoom.transform(svg, target);
          if (nodeSetRef.current) {
            cameras.set(cameraKeyRef.current, { nodeSet: nodeSetRef.current, transform: target });
          }
          return;
        }
        flightRef.current = { raf: requestAnimationFrame(step), target };
        zoom.transform(
          svg,
          at(c0x + (c1x - c0x) * e, c0y + (c1y - c0y) * e, k0 + (k1 - k0) * e)
        );
      };
      flightRef.current = { raf: requestAnimationFrame(step), target };
      // Stored up front, for the same reason the frames store the target.
      if (nodeSetRef.current) {
        cameras.set(cameraKeyRef.current, { nodeSet: nodeSetRef.current, transform: target });
      }
    },
    [positionOf, dims.width, dims.height, cancelFlight]
  );
  const flyToRef = useRef(flyTo);
  flyToRef.current = flyTo;

  // ═══ EXPERIMENT: SEMANTIC ZOOM ─ resize also changes effective scale ═══
  // The tier and the counter-scale depend on zoom x (css width / viewBox
  // width). A pane resize moves that ratio WITHOUT firing a zoom event, so the
  // stored scale silently goes stale and the text is sized for the old pane --
  // which is how a label ended up rendered at 60px. Recomputed here whenever
  // the measured size changes.
  useEffect(() => {
    const el = svgRef.current;
    if (!el) return;
    const vb = el.viewBox?.baseVal;
    const ratio = vb && vb.width ? el.getBoundingClientRect().width / vb.width : 1;
    const effective = d3.zoomTransform(el).k * ratio;
    if (!effective || !isFinite(effective)) return;
    const next = tierForScale(effective);
    setDetailTier((prev) => (prev === next ? prev : next));
    const quantised = Math.round(effective * 50) / 50;
    setZoomScale((prev) => (prev === quantised ? prev : quantised));
  }, [dims.width, dims.height]);
  // ═══ END EXPERIMENT ═══

  // Held in a ref so the retry loop below can call the latest fitToView without
  // taking it as a dependency -- see the note on the effect's dep list.
  const fitToViewRef = useRef(fitToView);
  fitToViewRef.current = fitToView;
  const openReadableRef = useRef(openReadable);
  openReadableRef.current = openReadable;

  // Auto-fit when the graph's SHAPE changes -- on first render, and whenever
  // claims are added or removed.
  //
  // Without this the graph opens at 1:1 with the root at top-centre, so a real
  // debate is mostly off-screen and the only clue is a scrollbar-less blank
  // pane. Fit-to-view already existed as a button, but a view you must repair
  // before you can read it should not need a button.
  //
  // Keyed on the SET of node ids (and which debate), not the array, so
  // ordinary re-renders -- a status change, a selection, the poll while an AI
  // job runs -- leave the viewer's own pan and zoom alone. And when the graph
  // is mounted again over the same claims, the camera the reader left is put
  // back instead of a fresh fit (see `cameras`).
  useLayoutEffect(() => {
    if (!nodeSet) return;
    cameraKeyRef.current = cameraKey;
    nodeSetRef.current = nodeSet;
    cancelFlight();

    const stored = cameras.get(cameraKey);
    if (stored && stored.nodeSet === nodeSet) {
      const svg = d3.select(svgRef.current);
      const zoom = zoomRef.current;
      if (svg.node() && zoom) {
        // Fires the zoom handler, so the semantic-zoom tier follows.
        zoom.transform(svg, stored.transform);
        return;
      }
    }

    // New claims, or none remembered: fit once. The zoom's "end" stores it.
    //
    // Fit synchronously first. useLayoutEffect runs after the DOM is updated
    // but before paint, so the nodes are already laid out and getBBox() has
    // real numbers -- which means the graph is never painted unfitted.
    const rootNow = d3.select(svgRef.current).select<SVGGElement>("g.graph-root").node();
    const boxNow = rootNow?.getBBox();
    if (boxNow && boxNow.width > 0 && boxNow.height > 0) {
      openReadableRef.current();
      return;
    }


    // Retried across a few frames rather than fitted once.
    //
    // getBBox() returns zeroes until the nodes have actually been laid out, and
    // a single requestAnimationFrame is not reliably late enough -- the first
    // attempt measured an empty box, bailed, and never tried again, which is
    // why the graph still opened unfitted. Retrying until the box has a size
    // costs a few frames and removes the race entirely.
    let frame = 0;
    let raf = 0;
    const attempt = () => {
      const rootNode = d3
        .select(svgRef.current)
        .select<SVGGElement>("g.graph-root")
        .node();
      const measured = rootNode?.getBBox();
      if (measured && measured.width > 0 && measured.height > 0) {
        openReadableRef.current();
        return;
      }
      if (++frame < 10) raf = requestAnimationFrame(attempt);
    };
    raf = requestAnimationFrame(attempt);
    return () => cancelAnimationFrame(raf);
    // Deliberately keyed on the camera and the node set ALONE.
    //
    // With fitToView in the deps this effect re-ran on every unrelated render,
    // and each cleanup cancelled the pending frame before it could fire -- so
    // the fit was scheduled repeatedly and never once executed. fitToView is
    // read through a ref instead, so the retry survives re-renders.
  }, [cameraKey, nodeSet]);

  // ── Click handler — selects the claim server-side (SELECT_CLAIM), which
  // moves the "current claim" highlight to it on next render.
  const handleNodeClick = useCallback(
    (node: GraphNode, e: React.MouseEvent) => {
      // Always selects. It used to toggle the local echo off on a second
      // click, but the server keeps the claim selected either way, and with
      // connections lit from the selection the toggle left the graph showing
      // the current claim with none of its links.
      setSelectedId(node.id);
      onNodeClick?.(node);
      // Recorded before the round trip, so the server's echo does not fly again.
      flownTo.set(cameraKey, node.id);
      flyTo(node.id);
      const syntheticId = `selectClaimNode_${node.id}`;
      handleEvent(e as any, { id: syntheticId }, "onClick", syntheticId);
    },
    [onNodeClick, cameraKey, flyTo]
  );

  // A card, an arrow key or Escape: select a claim or a pill the way a click
  // does (SELECT_CLAIM / SELECT_LINK), and fly to it.
  const selectAndFly = useCallback(
    (id: string) => {
      const node = nodeById.get(id);
      if (!node && !groupByKey.has(id)) return;
      setSelectedId(id);
      if (node) onNodeClick?.(node);
      flownTo.set(cameraKey, id);
      flyTo(id);
      const syntheticId = node ? `selectClaimNode_${id}` : `selectLinkNode_${id.slice(1)}`;
      handleEvent(null, { id: syntheticId }, "onClick", syntheticId);
    },
    [nodeById, groupByKey, onNodeClick, cameraKey, flyTo]
  );
  const selectAndFlyRef = useRef(selectAndFly);
  selectAndFlyRef.current = selectAndFly;

  // A selection that did not come from this graph -- the Brief's "Open in
  // debate", a claim picked in the text view -- flies the camera there when it
  // arrives. Runs after the camera effect above has restored or fitted, and
  // only once the pane has its measured size: the first render's 800x600
  // placeholder would centre the claim in the wrong place.
  useEffect(() => {
    const sel = component.selectedNodeId;
    if (!sel || !nodeSet || !(nodeById.has(sel) || groupByKey.has(sel))) return;
    const el = containerRef.current;
    if (!el || Math.abs(el.clientWidth - dims.width) > 1 || Math.abs(el.clientHeight - dims.height) > 1) return;
    const prev = flownTo.get(cameraKey);
    flownTo.set(cameraKey, sel);
    if (prev === sel) return;
    // First sight of this map, on its root: the opening view already anchors there.
    if (prev === undefined && sel === rootClaimId) return;
    flyToRef.current(sel);
  }, [component.selectedNodeId, cameraKey, nodeSet, nodeById, groupByKey, rootClaimId, dims.width, dims.height]);

  // ── What the selection is connected to ────────────────────────────
  const selectedClaimId = selectedId && nodeById.has(selectedId) ? selectedId : null;
  const focusId = selectedClaimId ?? (selectedId && groupByKey.has(selectedId) ? selectedId : null);
  const focus = useMemo(
    () => (flyMode ? focusOf(focusId, relationGroups) : null),
    [flyMode, focusId, relationGroups]
  );

  // Off-screen cards, drawn against the LIVE camera. Everything the zoom
  // handler needs is read through this ref, since that handler is attached once.
  const cardsHostRef = useRef<HTMLDivElement>(null);
  drawCardsRef.current = () => {
    const host = cardsHostRef.current;
    const el = svgRef.current;
    const pane = containerRef.current;
    if (!host || !el || !pane) return;
    const sel = focusId ? positionOf(focusId) : undefined;
    if (!sel || !focus || pane.clientWidth === 0) {
      placeConnectionCards(host, null, [], { width: 0, height: 0 }, () => {});
      return;
    }
    const vb = el.viewBox?.baseVal;
    const ratio = vb && vb.width ? el.getBoundingClientRect().width / vb.width : 1;
    const t = d3.zoomTransform(el);
    const toPane = (n: { x?: number; y?: number }) => ({ x: t.applyX(n.x ?? 0) * ratio, y: t.applyY(n.y ?? 0) * ratio });
    const width = pane.clientWidth, height = pane.clientHeight;
    const targets: CardTarget[] = [];
    for (const c of focus.connections) {
      const n = nodeById.get(c.id);
      if (!n) continue;
      // Every connection gets a card, in view or not: the cards are the
      // selected claim's list of links, each sitting at the pane edge in
      // its claim's direction, so the list stays put while you read.
      const p = toPane(n);
      targets.push({ id: c.id, kind: c.kind, color: statusColorOf(n), who: n.creatorUsername || `#${n.id}`, text: n.text, ...p });
    }
    placeConnectionCards(host, toPane(sel), targets, { width, height }, (id) => selectAndFlyRef.current(id));
  };
  useLayoutEffect(() => {
    drawCardsRef.current();
  });

  // ── Keyboard: walk the map ────────────────────────────────────────
  // Live while the pane has focus or the pointer is over it. Arrow keys and
  // Escape select through SELECT_CLAIM, like a click; F is the Fit button.
  const hoverRef = useRef(false);
  const keyHandlerRef = useRef<(e: KeyboardEvent) => void>(() => {});
  keyHandlerRef.current = (e: KeyboardEvent) => {
    const pane = containerRef.current;
    if (!pane || e.defaultPrevented || e.ctrlKey || e.metaKey || e.altKey) return;
    if (pane.clientWidth === 0) return; // the text view is showing
    const target = e.target as HTMLElement | null;
    if (target && (target.isContentEditable || /^(INPUT|TEXTAREA|SELECT)$/.test(target.tagName))) return;
    if (!hoverRef.current && !pane.contains(document.activeElement)) return;

    const current = focusId ?? rootClaimId;
    let next: string | undefined;
    if (e.key === "ArrowUp" || e.key === "ArrowDown" || e.key === "ArrowLeft" || e.key === "ArrowRight") {
      const nav = navigationFor(current, relationGroups, nodeById);
      next =
        e.key === "ArrowUp" ? nav.parent :
        e.key === "ArrowDown" ? nav.firstChild :
        e.key === "ArrowLeft" ? nav.prevSibling : nav.nextSibling;
      e.preventDefault(); // even at a dead end, the page should not scroll
    } else if (e.key === "Escape") {
      next = rootClaimId;
      e.preventDefault();
    } else if (e.key === "f" || e.key === "F") {
      e.preventDefault();
      fitToViewRef.current();
      return;
    } else {
      return;
    }
    if (next) selectAndFlyRef.current(next);
  };
  useEffect(() => {
    const onKey = (e: KeyboardEvent) => keyHandlerRef.current(e);
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, []);

  // ── Junction click — selects the ARGUMENT server-side (SELECT_LINK). The
  // group key is `r<relationId>`; what goes to the server is the relation id,
  // which is also what the challenge handler takes if the user then challenges
  // it. Same local echo as a node click, so the ring moves before the round
  // trip lands.
  const handleJunctionClick = useCallback(
    (key: string, e: React.MouseEvent) => {
      e.stopPropagation();
      // Always selects, like a claim click: the server keeps the pill
      // selected either way.
      setSelectedId(key);
      flownTo.set(cameraKey, key);
      flyTo(key);
      const syntheticId = `selectLinkNode_${key.slice(1)}`;
      handleEvent(e as any, { id: syntheticId }, "onClick", syntheticId);
    },
    [cameraKey, flyTo]
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
        outline: "none",
      }}
      ref={containerRef}
      // Focusable, so the arrow keys work after a click on the map.
      tabIndex={0}
      onPointerEnter={() => { hoverRef.current = true; }}
      onPointerLeave={() => { hoverRef.current = false; }}
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
            <polygon points="0 0, 10 3.5, 0 7" fill="var(--text-3)" />
          </marker>
          <marker
            id={`${component.id}-arrow-challenge`}
            markerWidth="10"
            markerHeight="7"
            refX="9"
            refY="3.5"
            orient="auto"
          >
            <polygon points="0 0, 10 3.5, 0 7" fill="var(--challenge)" />
          </marker>
          <marker
            id={`${component.id}-arrow-support`}
            markerWidth="10"
            markerHeight="7"
            refX="9"
            refY="3.5"
            orient="auto"
          >
            <polygon points="0 0, 10 3.5, 0 7" fill="var(--settled)" />
          </marker>
        </defs>

        <g className="graph-root">
          {/* ── Relation lines. All lines first, so every node -- claim box or
              pill -- is drawn on top of them and hides the line ends. That is
              what makes arrowheads unnecessary: rows carry the direction. */}
          {relationGroups.map((group) => {
            const isChallenge = isChallengeEdge(group.type);
            const color = isChallenge
              ? EDGE_COLORS.challenges
              : isSupportEdge(group.type)
                ? EDGE_COLORS.PARENT_CHILD
                : EDGE_COLORS.NORMAL;
            // The selected claim's connections light up: the accent, or a
            // brighter red for a challenge. Restyling only; nothing moves.
            const litColor = isChallenge ? EDGE_COLORS.challenges : "var(--accent)";
            const lit = (end: string) => !!focus?.litLines.has(lineKey(group.key, end));
            return (
              <g key={`lines-${group.key}`}>
                {/* Every source feeds INTO the pill: a plain segment, the same
                    for evidence into a support and a challenger into a challenge. */}
                {group.sources.map((source) => (
                  <line
                    key={source.id}
                    x1={source.x ?? 100}
                    y1={source.y ?? 100}
                    x2={group.x}
                    y2={group.y}
                    stroke={lit(source.id) ? litColor : color}
                    strokeWidth={lit(source.id) ? 2.5 : 1.5}
                    strokeOpacity={lit(source.id) ? 1 : 0.7}
                  />
                ))}
                {/* The pill's own segment, into whatever it is about: a claim,
                    or another pill. Dashed for a challenge. */}
                <line
                  x1={group.x}
                  y1={group.y}
                  x2={group.tx}
                  y2={group.ty}
                  stroke={lit("out") ? litColor : color}
                  strokeWidth={(isChallenge ? 2.5 : 1.5) + (lit("out") ? 1 : 0)}
                  strokeOpacity={lit("out") ? 1 : 0.7}
                  strokeDasharray={isChallenge ? "6,3" : "none"}
                />
              </g>
            );
          })}

          {/* ── Pills (relation nodes) ─────────────────────────────── */}
          {relationGroups.map((group) => {
            const isChallenge = isChallengeEdge(group.type);
            const color = isChallenge
              ? EDGE_COLORS.challenges
              : isSupportEdge(group.type)
                ? EDGE_COLORS.PARENT_CHILD
                : EDGE_COLORS.NORMAL;
            // ═══ EXPERIMENT: SEMANTIC ZOOM ─ basis labels vanish on the map ═══
            const label = semanticZoom && detailTier === "map" ? "" : group.label;
            const jw = label.length * JUNCTION_CHAR_WIDTH + JUNCTION_PADDING_X * 2;
            // A selected pill wears the same solid yellow outline a current
            // claim does. Its own colour (support/challenge) stays in the text.
            const isSelectedJunction = selectedId === group.key;
            const junctionStroke = isSelectedJunction ? CURRENT_STROKE : color;
            const junctionStrokeWidth = isSelectedJunction ? 3 : 1.5;
            const ringW = (label ? jw : JUNCTION_DOT_RADIUS * 2) + 10;
            const ringH = (label ? JUNCTION_HEIGHT : JUNCTION_DOT_RADIUS * 2) + 10;

            return (
              <g
                key={group.key}
                transform={`translate(${group.x},${group.y})`}
                className="graph-junction"
                opacity={focus && !focus.litGroups.has(group.key) && !isSelectedJunction ? 0.45 : 1}
                style={{ cursor: group.selectable ? "pointer" : "default", transition: "opacity 150ms" }}
                onClick={(e) => {
                  if (group.selectable) handleJunctionClick(group.key, e);
                }}
              >
                {/* Hit area. A pill is ~20px tall and shrinks with the zoom;
                    without padding it is unclickable at Fit. Transparent, not
                    "none": it must catch the click and draw nothing. */}
                <rect
                  x={-jw / 2 - 8}
                  y={-JUNCTION_HEIGHT / 2 - 8}
                  width={jw + 16}
                  height={JUNCTION_HEIGHT + 16}
                  fill="transparent"
                />
                {/* Undercut for the viewer: this argument carries nothing for
                    them until the challenge against it is answered. */}
                {group.undercutForViewer && (
                  <rect
                    x={-ringW / 2}
                    y={-ringH / 2}
                    width={ringW}
                    height={ringH}
                    rx={ringH / 2}
                    fill="none"
                    stroke={EDGE_COLORS.challenges}
                    strokeWidth={2}
                    strokeDasharray="4,3"
                  />
                )}
                {label ? (
                  <>
                    <rect
                      x={-jw / 2}
                      y={-JUNCTION_HEIGHT / 2}
                      width={jw}
                      height={JUNCTION_HEIGHT}
                      rx={JUNCTION_HEIGHT / 2}
                      fill="var(--bg)"
                      stroke={junctionStroke}
                      strokeWidth={junctionStrokeWidth}
                    />
                    <text
                      textAnchor="middle"
                      dominantBaseline="central"
                      fontSize={9}
                      fontWeight={600}
                      fill={color}
                      style={{ pointerEvents: "none", letterSpacing: "0.03em" }}
                    >
                      {label}
                    </text>
                  </>
                ) : (
                  // No basis chosen: a plain dot. Still a node -- it has to be,
                  // so sources converge on it and so it can be a target.
                  <circle
                    r={JUNCTION_DOT_RADIUS}
                    fill="var(--bg)"
                    stroke={junctionStroke}
                    strokeWidth={junctionStrokeWidth}
                  />
                )}
              </g>
            );
          })}

          {/* ── Nodes ─────────────────────────────────────────────── */}
          {positionedNodes.map((node) => {
            const cx = node.x ?? 100;
            const cy = node.y ?? 100;
            const w = node.isRoot ? ROOT_WIDTH : NODE_WIDTH;
            const h = node.isRoot ? ROOT_HEIGHT : NODE_HEIGHT;
            const isSelected = node.id === selectedId;
            // Selecting a pill also focuses the claim it argues for (so the
            // panel and modal have it), but only ONE thing should wear the
            // highlight -- the pill. The claim's own colours stay.
            const linkSelected = !!selectedId && selectedId.startsWith("r");
            const isCurrent = node.isCurrent && !linkSelected;
            const colors = STATUS_COLORS[node.status || ""] || DEFAULT_NODE_COLOR;
            const isChallenge = node.type === "challenge";
            // Challenge (root) claims get an orange-tinted body and border; the
            // current/selected highlight still takes precedence when active.
            const fill = isChallenge ? "var(--challenge-panel)" : colors.fill;
            const stroke = isCurrent ? CURRENT_STROKE : restingStroke(node);
            const strokeWidth = isCurrent ? 4 : isSelected ? 3 : isChallenge ? 3 : 2;
            // ═══════════ EXPERIMENT: SEMANTIC ZOOM (per-node detail) ═══════════
            // `tier` is "full" whenever the experiment is off, so every branch
            // below collapses to the original behaviour.
            const tier: DetailTier = semanticZoom ? detailTier : "full";

            // Text is COUNTER-SCALED below "full": the font grows as the graph
            // shrinks, so a label stays about the same size on screen however
            // far you zoom out. Simply hiding text made the zoomed-out view
            // legible-by-omission, which is not the same as useful -- you could
            // see the shape of the argument but not what any of it said.
            //
            // The trade is characters, not pixels: bigger glyphs in a fixed
            // node mean fewer of them, so the label shortens as you zoom out
            // rather than becoming unreadable.
            const effScale = semanticZoom ? Math.max(zoomScale, 0.05) : 1;
            // The cap matters: counter-scaling only works while the glyphs
            // still fit the node, and the node is shrinking too. Past 6x the
            // box is a few screen pixels wide and no font size rescues it.
            const fontScale = tier === "full" ? 1 : Math.min(1 / effScale, 6);
            const bodyFontSize = 10 * fontScale;
            // Once the counter-scale is capped out, the on-screen size starts
            // falling again. Below ~7px the label is noise rather than
            // information, so drop it and let colour and shape carry the view.
            const labelLegible = tier === "full" || bodyFontSize * effScale >= 7;
            // 0.55em is about the average glyph width for this face.
            const charsThatFit = Math.max(
              4,
              Math.floor((node.isRoot ? ROOT_WIDTH : NODE_WIDTH) / (bodyFontSize * 0.55)) - 1
            );
            const lines = !labelLegible
              ? []
              : tier === "full"
                ? wrapText(node.text, CHARS_PER_LINE, MAX_LINES)
                : wrapText(node.text, charsThatFit, 2);
            // ═══════════ END EXPERIMENT (per-node detail) ═══════════
            // Center the whole block (sentence + creator + status) based on
            // this node's actual line count, instead of reserving fixed
            // room for MAX_LINES -- keeps everything close together
            // regardless of how long the sentence is.
            const sentenceGap = 4;
            const lineH = tier === "full" ? LINE_HEIGHT : LINE_HEIGHT * fontScale;
            const contentHeight =
              tier === "full"
                ? lines.length * LINE_HEIGHT + sentenceGap + 2 * LINE_HEIGHT
                : lines.length * lineH;
            const sentenceTop = -contentHeight / 2 + lineH * 0.8;
            const creatorY = sentenceTop + (lines.length - 1) * LINE_HEIGHT + sentenceGap + LINE_HEIGHT;
            const statusY = creatorY + LINE_HEIGHT;
            const statusLabel =
              node.status === "TRUE_CLAIM" ? "Status: True" :
              node.status === "FALSE_CLAIM" ? "Status: False" :
              "Status: Undetermined";
            const statusColor = statusColorOf(node);

            // Claims unrelated to the selection step back.
            const dimmed = !!focus && !isSelected && !focus.connectedIds.has(node.id);
            return (
              <g
                key={node.id}
                className="graph-node"
                data-claim-id={node.id}
                opacity={dimmed ? 0.45 : 1}
                style={{ cursor: "pointer", transition: "opacity 150ms" }}
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
                    stroke={isCurrent ? CURRENT_STROKE : colors.stroke}
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
                  fontSize={bodyFontSize}
                  fontWeight="500"
                  style={{ pointerEvents: "none", userSelect: "none" }}
                >
                  {lines.map((line, i) => (
                    <tspan key={i} x={0} y={sentenceTop + i * lineH}>
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
                      fill="var(--bg)"
                      style={{ userSelect: "none" }}
                    >
                      {node.creatorUsername.charAt(0).toUpperCase()}
                    </text>
                  </g>
                )}
                {/* Created by / Status — only at "full".
                    ═══ EXPERIMENT: SEMANTIC ZOOM ═══
                    At 8px these are the first things to become illegible, and
                    both are already carried by other channels: the author by
                    the coloured badge above, the status by the node's own
                    fill and border. Dropping them loses no information at a
                    scale where they could not be read anyway. */}
                {tier === "full" && (
                  <>
                    <text
                      textAnchor="middle"
                      y={creatorY}
                      fill="var(--text-muted)"
                      fontSize="8"
                      style={{ pointerEvents: "none" }}
                    >
                      By: {node.creatorUsername || `#${node.id}`}
                    </text>
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
                  </>
                )}
              </g>
            );
          })}
        </g>
      </svg>

      {/* Off-screen connection cards. Filled by placeConnectionCards, never
          by React: they move with the camera on every frame. */}
      <div ref={cardsHostRef} className="graph-cards" />

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
          zIndex: 3,
        }}
      >
        <button
          onClick={toggleFlyMode}
          className="graph-zoom-btn"
          title={
            flyMode
              ? "Fly-to ON: a click flies to the claim and shows its connections — click to turn off"
              : "Fly-to OFF: a click only selects — click to turn on"
          }
          aria-pressed={flyMode}
          style={{
            width: "28px",
            height: "28px",
            display: "flex",
            alignItems: "center",
            justifyContent: "center",
            background: flyMode ? "var(--accent-soft)" : "var(--surface-elevated)",
            border: `1px solid ${flyMode ? "var(--accent)" : "var(--border-default)"}`,
            borderRadius: "var(--radius-sm)",
            cursor: "pointer",
            color: flyMode ? "var(--accent-hover)" : "var(--text-secondary)",
            fontSize: "0.8125rem",
          }}
        >
          ⌖
        </button>
        {/* ═══════════ EXPERIMENT: SEMANTIC ZOOM (toggle) ═══════════
            Delete this button to remove the control; the feature then sits
            permanently at SEMANTIC_ZOOM_DEFAULT. */}
        <button
          onClick={() => setSemanticZoom((v) => !v)}
          className="graph-zoom-btn"
          title={
            semanticZoom
              ? `Semantic zoom ON (${detailTier}) — click for classic view`
              : "Semantic zoom OFF — click to simplify nodes when zoomed out"
          }
          style={{
            width: "28px",
            height: "28px",
            display: "flex",
            alignItems: "center",
            justifyContent: "center",
            background: semanticZoom ? "var(--accent-soft)" : "var(--surface-elevated)",
            border: `1px solid ${semanticZoom ? "var(--accent)" : "var(--border-default)"}`,
            borderRadius: "var(--radius-sm)",
            cursor: "pointer",
            color: semanticZoom ? "var(--accent-hover)" : "var(--text-secondary)",
            fontSize: "0.8125rem",
          }}
        >
          ⌸
        </button>
        {/* ═══════════ END EXPERIMENT (toggle) ═══════════ */}
        <button
          onClick={() => {
            const svg = d3.select(svgRef.current);
            const zoom = zoomRef.current;
            if (!svg.node() || !zoom) return;
            cancelFlight();
            svg
              .transition()
              .duration(300)
              .call(zoom.transform, d3.zoomIdentity.translate(0, 0).scale(1));
          }}
          className="graph-zoom-btn"
          title="Reset zoom to 100%"
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
          onClick={() => fitToView()}
          className="graph-zoom-btn"
          title="Fit whole debate in view"
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

      {/* Key: a button, and the legend under it while open */}
      <button
        type="button"
        onClick={toggleKey}
        className="graph-zoom-btn"
        aria-expanded={keyOpen}
        title={keyOpen ? "Hide the key" : "Show what the colours and lines mean"}
        style={{
          position: "absolute",
          top: "0.75rem",
          left: "0.75rem",
          height: "28px",
          padding: "0 0.6rem",
          gap: "0.35rem",
          background: keyOpen ? "var(--accent-soft)" : "var(--surface-elevated)",
          border: `1px solid ${keyOpen ? "var(--accent)" : "var(--border-default)"}`,
          borderRadius: "var(--radius-sm)",
          color: keyOpen ? "var(--accent-hover)" : "var(--text-secondary)",
          fontSize: "0.75rem",
        }}
      >
        <span aria-hidden="true">{keyOpen ? "▾" : "▸"}</span>
        Key
      </button>
      {keyOpen && (
      <div
        style={{
          position: "absolute",
          top: "calc(0.75rem + 32px)",
          left: "0.75rem",
          maxWidth: "min(28rem, calc(100% - 1.5rem))",
          display: "flex",
          flexWrap: "wrap",
          gap: "0.5rem",
          fontSize: "0.6875rem",
          color: "var(--text-muted)",
          background: "var(--panel)",
          padding: "0.375rem 0.5rem",
          borderRadius: "var(--radius-sm)",
          border: "1px solid var(--border-subtle)",
        }}
      >
        <span style={{ display: "flex", alignItems: "center", gap: "0.25rem" }}>
          <span style={{ width: 8, height: 8, borderRadius: "50%", background: "var(--settled)", display: "inline-block" }} />
          True
        </span>
        <span style={{ display: "flex", alignItems: "center", gap: "0.25rem" }}>
          <span style={{ width: 8, height: 8, borderRadius: "50%", background: "var(--open)", display: "inline-block" }} />
          False
        </span>
        <span style={{ display: "flex", alignItems: "center", gap: "0.25rem" }}>
          <span style={{ width: 8, height: 8, borderRadius: "50%", background: "var(--text-3)", display: "inline-block" }} />
          Undetermined
        </span>
        <span style={{ display: "flex", alignItems: "center", gap: "0.25rem" }}>
          <span style={{ width: 12, height: 0, borderTop: "2px dashed var(--challenge)", display: "inline-block" }} />
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
      )}
    </div>
  );
};

export default DebateGraph;
