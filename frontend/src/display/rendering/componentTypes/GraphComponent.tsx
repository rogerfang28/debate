import React, { useEffect, useLayoutEffect, useRef, useState, useCallback, useMemo } from "react";
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
  basis?: string;      // EVIDENTIAL | AUTHORITY | DEFINITIONAL | NORMATIVE, or empty
  relationId?: number; // edges sharing this belong to ONE relation; 0/absent = its own
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

// ── Junction (relation) nodes ─────────────────────────────────────────
// A relation is drawn as its own small node sitting on the edge: every
// source points AT the junction, and the junction points at the target.
// The junction is what carries the relation's meaning (its basis), and it
// is the thing an attack on the reasoning would eventually target.
//
// Junctions are placed at the MIDPOINT of the edge rather than being
// inserted into the tree layout as real levels. Making them layout nodes
// would double the depth of every tree and force the row spacing, subtree
// widths and root detection to all be reworked -- for a node that carries
// no children of its own. Positioning them geometrically keeps the tree
// exactly as it was and costs nothing.
const JUNCTION_HEIGHT = 18;
// Radius of the dot drawn when a relation has no basis. Small enough to read
// as a joint in the line rather than as a claim of its own.
const JUNCTION_DOT_RADIUS = 4;
const JUNCTION_CHAR_WIDTH = 6.2;
const JUNCTION_PADDING_X = 8;

// Relations created before the basis dropdown existed carry a fixed string in
// the same column -- one for support, one for challenge. Neither is a basis
// and neither may be displayed as one; see RelationBuilder, which deliberately
// does not launder them into real values.
const PLACEHOLDER_BASES = ["parent to child connection", "challenge link"];

// Edge-type predicates.
//
// "SUPPORTS" is what the backend emits now. "PARENT_CHILD" and "supports" are
// accepted too: the first is the pre-relation name for the same edge, and a
// stale cached page or a saved layout JSON can still carry it. Treating an
// unrecognised type as support would silently mis-draw the tree, so the check
// is an allow-list rather than "not a challenge".
function isSupportEdge(type?: string): boolean {
  return type === "SUPPORTS" || type === "PARENT_CHILD" || type === "supports";
}

function isChallengeEdge(type?: string): boolean {
  return type === "CHALLENGE" || type === "challenges" || type === "OPPOSES";
}

// What the junction says: its BASIS, or nothing.
//
// It deliberately does NOT fall back to the role. Support versus opposition is
// already carried by colour -- grey for support, orange and dashed for a
// challenge -- so writing "SUPPORTS" on the node repeats what the edge already
// says. Worse, it puts two different vocabularies in one slot: a reader seeing
// EVIDENTIAL on one junction and SUPPORTS on another would take them for
// alternatives in the same list, when one is a basis and the other is a role.
//
// An empty label means no basis was ever chosen -- true of every relation
// created before the basis dropdown existed. Those render as a small plain
// dot, which reads as "a relation, unspecified" rather than claiming a basis
// nobody selected.
function junctionLabel(edge: GraphEdge): string {
  const basis = (edge.basis ?? "").trim();
  if (basis && !PLACEHOLDER_BASES.includes(basis)) return basis;
  return "";
}
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
  // Only the TIER is kept in state, not the raw scale: re-rendering 22 nodes
  // on every wheel tick would be wasteful, and nothing here cares about the
  // exact number -- only which of three bands it falls in.
  const [detailTier, setDetailTier] = useState<DetailTier>("full");
  // The scale itself is needed to counter-scale text. Quantised so a wheel
  // gesture re-renders a handful of times rather than on every tick.
  const [zoomScale, setZoomScale] = useState(1);
  // ═══════════ END EXPERIMENT (state) ═══════════
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

    // Build the children map from SUPPORT edges.
    //
    // A support edge runs SUPPORTER -> SUPPORTED, which is UP the tree: the
    // evidence points at the conclusion it backs. The layout draws top-down,
    // so the ends are inverted here -- the supported claim owns its supporters
    // as children.
    //
    // Root detection still works unchanged and for the right reason: the root
    // is the claim that supports nothing, which is exactly the node that is
    // never a source.
    const childrenMap = new Map<string, string[]>();
    const hasParent = new Set<string>();
    for (const e of edges) {
      // BOTH support and challenge edges give layout structure. A challenge
      // claim has no parent-child path from the root, so leaving challenges out
      // here drops every one of them from the tree -- they end up unpositioned
      // and are never drawn.
      //
      // The two map identically because both already run source -> target with
      // the source underneath: a supporter sits below what it supports, and a
      // challenger below what it attacks.
      if (!isSupportEdge(e.type) && !isChallengeEdge(e.type)) continue;

      if (!childrenMap.has(e.target)) childrenMap.set(e.target, []);
      // Guard against the same pair arriving twice (it did, until the backend
      // stopped emitting a phantom support edge beside every challenge) --
      // a duplicate would place the node twice and double the subtree width.
      if (!childrenMap.get(e.target)!.includes(e.source)) {
        childrenMap.get(e.target)!.push(e.source);
      }
      hasParent.add(e.source);
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
  //
  // The behaviour is kept in a ref so every caller drives the SAME one.
  // Previously the fit and reset buttons each constructed a throwaway
  // d3.zoom() and called .transform on it, which moves the picture but leaves
  // the ATTACHED behaviour still holding the old transform -- so the next drag
  // or wheel snapped back to wherever the graph used to be.
  const zoomRef = useRef<d3.ZoomBehavior<SVGSVGElement, unknown> | null>(null);

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
      .on("zoom", (event) => {
        svg.select("g.graph-root").attr("transform", event.transform.toString());
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
      });

    zoomRef.current = zoom;
    svg.call(zoom);
  }, []);

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
  }, [dims.width, dims.height]);

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
  // Keyed on the node ids rather than the array, so ordinary re-renders (a
  // status change, a selection) leave the viewer's own pan and zoom alone.
  const shapeKey = positionedNodes.map((n) => n.id).join(",");
  useLayoutEffect(() => {
    if (!shapeKey) return;

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
    // Deliberately keyed on shapeKey ALONE.
    //
    // With fitToView in the deps this effect re-ran on every unrelated render,
    // and each cleanup cancelled the pending frame before it could fire -- so
    // the fit was scheduled repeatedly and never once executed. fitToView is
    // read through a ref instead, so the retry survives re-renders.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [shapeKey]);

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

  // ── Junction click — selects the ARGUMENT server-side (SELECT_LINK). The
  // group key is `r<relationId>`; what goes to the server is the relation id,
  // which is also what the challenge handler takes if the user then challenges
  // it. Same local echo as a node click, so the ring moves before the round
  // trip lands.
  const handleJunctionClick = useCallback(
    (key: string, e: React.MouseEvent) => {
      e.stopPropagation();
      setSelectedId(key === selectedId ? null : key);
      const syntheticId = `selectLinkNode_${key.slice(1)}`;
      handleEvent(e as any, { id: syntheticId }, "onClick", syntheticId);
    },
    [selectedId]
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

  // ── Group edges into the relations that produced them ────────────
  //
  // A relation with several from-claims arrives as several edges sharing a
  // relationId, and must draw ONE junction with several incoming segments.
  // That grouping is the whole point: N edges into one target could mean
  // "these N together support it" (one relation) or "each of these
  // independently supports it" (N relations), and the picture has to tell
  // those apart. Without the id they are indistinguishable.
  const relationGroups = useMemo(() => {
    const byKey = new Map<
      string,
      {
        key: string;
        sources: GraphNode[];
        target: GraphNode;
        type?: string;
        label: string;
        // Only an ATTRIBUTED relation (a real relation id) can be selected:
        // the id is what the server needs to know which argument was meant.
        selectable: boolean;
      }
    >();

    for (const edge of edges) {
      const source = nodeMap.get(edge.source);
      const target = nodeMap.get(edge.target);
      // Endpoints outside the laid-out tree (an unreachable claim) have no
      // position, so there is nothing to draw between.
      if (!source || !target) continue;

      // relationId 0/absent means the backend could not attribute this edge.
      // Those stand alone rather than being merged with each other -- lumping
      // unattributed edges together would invent a relation nobody asserted.
      const key =
        edge.relationId && edge.relationId > 0
          ? `r${edge.relationId}`
          : `e${edge.id}`;

      const existing = byKey.get(key);
      if (!existing) {
        byKey.set(key, {
          key,
          sources: [source],
          target,
          type: edge.type,
          label: junctionLabel(edge),
          selectable: !!(edge.relationId && edge.relationId > 0),
        });
        continue;
      }
      // Same relation, another from-claim. Guard against a repeated endpoint
      // so a duplicated edge cannot draw two identical segments.
      if (!existing.sources.some((s) => s.id === source.id)) {
        existing.sources.push(source);
      }
    }

    return [...byKey.values()];
  }, [edges, positionedNodes]);

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
          {/* ── Relations (edges + junction nodes) ────────────────── */}
          {relationGroups.map((group) => {
            const target = group.target;
            const tx = target.x ?? 200;
            const ty = target.y ?? 200;

            const isChallenge = isChallengeEdge(group.type);
            const color = isChallenge
              ? EDGE_COLORS.challenges
              : isSupportEdge(group.type)
                ? EDGE_COLORS.PARENT_CHILD
                : EDGE_COLORS.NORMAL;
            const dash = isChallenge ? "6,3" : "none";
            const strokeWidth = isChallenge ? 2.5 : 1.5;

            // The junction sits midway between the sources' centre of mass and
            // the target. With a single source that is exactly the edge
            // midpoint, so one-from relations render precisely as they did
            // before grouping existed.
            const n = group.sources.length;
            const cx = group.sources.reduce((s, node) => s + (node.x ?? 100), 0) / n;
            const cy = group.sources.reduce((s, node) => s + (node.y ?? 100), 0) / n;
            const jx = (cx + tx) / 2;
            const jy = (cy + ty) / 2;

            // ═══ EXPERIMENT: SEMANTIC ZOOM ─ basis labels vanish on the map ═══
            // At map scale a basis renders around 3px. The colour already says
            // support vs challenge, so the pill becomes a dot like any other
            // unlabelled relation.
            const label =
              semanticZoom && detailTier === "map" ? "" : group.label;
            const jw = label.length * JUNCTION_CHAR_WIDTH + JUNCTION_PADDING_X * 2;
            // A selected pill wears the same solid yellow outline a current
            // claim does, so "what is selected" reads the same for both kinds
            // of node. Its own colour (support/challenge) stays in the text.
            const isSelectedJunction = selectedId === group.key;
            const junctionStroke = isSelectedJunction ? "#fbbf24" : color;
            const junctionStrokeWidth = isSelectedJunction ? 3 : 1.5;

            return (
              <g key={group.key}>
                {/* Every source feeds INTO the junction. No arrowheads here --
                    the relation as a whole makes one claim about the target,
                    so only the junction's outgoing segment is an assertion. */}
                {group.sources.map((source) => (
                  <line
                    key={source.id}
                    x1={source.x ?? 100}
                    y1={source.y ?? 100}
                    x2={jx}
                    y2={jy}
                    stroke={color}
                    strokeWidth={strokeWidth}
                    strokeOpacity={0.7}
                    strokeDasharray={dash}
                    style={{ transition: "x1 0.05s, y1 0.05s, x2 0.05s, y2 0.05s" }}
                  />
                ))}
                <line
                  x1={jx}
                  y1={jy}
                  x2={tx}
                  y2={ty}
                  stroke={color}
                  strokeWidth={strokeWidth}
                  strokeOpacity={0.7}
                  strokeDasharray={dash}
                  markerEnd={`url(#${component.id}-${isChallenge ? "arrow-challenge" : "arrow-support"})`}
                  style={{ transition: "x1 0.05s, y1 0.05s, x2 0.05s, y2 0.05s" }}
                />
                <g
                  transform={`translate(${jx},${jy})`}
                  className="graph-junction"
                  style={{ cursor: group.selectable ? "pointer" : "default" }}
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
                  {label ? (
                    <>
                      <rect
                        x={-jw / 2}
                        y={-JUNCTION_HEIGHT / 2}
                        width={jw}
                        height={JUNCTION_HEIGHT}
                        rx={JUNCTION_HEIGHT / 2}
                        fill="#0f172a"
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
                    // No basis chosen: a plain dot. The relation is still a
                    // node -- it has to be, so several sources can converge on
                    // it and so it can be a target later -- but it makes no
                    // claim about what KIND of connection it is. Support
                    // versus opposition is already carried by the colour.
                    <circle
                      r={JUNCTION_DOT_RADIUS}
                      fill="#0f172a"
                      stroke={junctionStroke}
                      strokeWidth={junctionStrokeWidth}
                    />
                  )}
                </g>
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
            const fill = isChallenge ? "#3a2412" : colors.fill;
            const stroke = isCurrent ? "#fbbf24" : isChallenge ? "#f97316" : colors.stroke;
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
                      fill="#0f1117"
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
                      Created by: {node.creatorUsername || `#${node.id}`}
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
            background: semanticZoom ? "var(--accent-soft, #1e3a5f)" : "var(--surface-elevated)",
            border: `1px solid ${semanticZoom ? "#60a5fa" : "var(--border-default)"}`,
            borderRadius: "var(--radius-sm)",
            cursor: "pointer",
            color: semanticZoom ? "#60a5fa" : "var(--text-secondary)",
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
