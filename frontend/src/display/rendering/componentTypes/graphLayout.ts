// graphLayout.ts -- the debate graph's data shapes and its layout.
//
// Pure: no React, no DOM, no d3. The component renders what this returns.
// Kept separate so the one rule that places everything -- a node's children
// are the things that point at it -- can be read, and tested, on its own.

export interface GraphNode {
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

export interface GraphEdge {
  id: string;
  source: string;
  target: string;
  type?: string;       // "supports" | "challenges" | "counters" | "PARENT_CHILD" | "CHALLENGE"
  basis?: string;      // EVIDENTIAL | AUTHORITY | DEFINITIONAL | NORMATIVE, or empty
  relationId?: number; // edges sharing this belong to ONE relation; 0/absent = its own
  // A challenge aimed at an ARGUMENT ends at that relation's junction, not at
  // `target` (which still names the claim the argument was for, for placement).
  targetRelationId?: number;
}

// ── Junction (relation) nodes ─────────────────────────────────────────
// A relation is drawn as its own small node sitting on the edge: every
// source points AT the junction, and the junction points at the target.
// The junction is what carries the relation's meaning (its basis), and it
// is the thing an attack on the reasoning would eventually target.
//
// Junctions ARE layout nodes, in rows of their own between claim rows -- see
// layoutDebate. They used to be painted at the midpoint of the edge, which
// worked until a relation could be the TARGET of a challenge: then nothing
// could hang off it and undercuts had nowhere to go.
export const JUNCTION_HEIGHT = 18;
// Radius of the dot drawn when a relation has no basis. Small enough to read
// as a joint in the line rather than as a claim of its own.
export const JUNCTION_DOT_RADIUS = 4;
export const JUNCTION_CHAR_WIDTH = 6.2;
export const JUNCTION_PADDING_X = 8;

// Relations created before the basis dropdown existed carry a fixed string in
// the same column -- one for support, one for challenge. Neither is a basis
// and neither may be displayed as one; see RelationBuilder, which deliberately
// does not launder them into real values.
export const PLACEHOLDER_BASES = ["parent to child connection", "challenge link"];

// Edge-type predicates.
//
// "SUPPORTS" is what the backend emits now. "PARENT_CHILD" and "supports" are
// accepted too: the first is the pre-relation name for the same edge, and a
// stale cached page or a saved layout JSON can still carry it. Treating an
// unrecognised type as support would silently mis-draw the tree, so the check
// is an allow-list rather than "not a challenge".
export function isSupportEdge(type?: string): boolean {
  return type === "SUPPORTS" || type === "PARENT_CHILD" || type === "supports";
}

export function isChallengeEdge(type?: string): boolean {
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
export function junctionLabel(edge: GraphEdge): string {
  const basis = (edge.basis ?? "").trim();
  if (basis && !PLACEHOLDER_BASES.includes(basis)) return basis;
  return "";
}

// One drawn relation: the pill, its sources, and where its line ends.
export interface RelationGroup {
  key: string;                 // `r<relationId>`, or `e<edgeId>` when unattributed
  sources: GraphNode[];
  targetId: string;            // the claim this relation is about (for placement)
  type?: string;
  label: string;
  selectable: boolean;         // only an attributed relation can be selected
  targetRelationKey?: string;  // set for an undercut: the pill this one attacks
  x: number;                   // the pill's centre
  y: number;
  tx: number;                  // where the pill's line ends: a claim or a pill centre
  ty: number;
  undercutForViewer: boolean;  // some challenge aimed at THIS pill is TRUE for the viewer
}

// ── Layout: claims AND relations are nodes ─────────────────────────────
//
// One rule places everything: a node's children are the things that point at
// it. A claim's children are the pills that argue for or against it; a pill's
// children are its source claims and the pills that undercut it. Rows
// alternate claims / pills, so a pill row is short and a claim row is tall.
//
// That is what makes a challenge to an ARGUMENT look exactly like a challenge
// to a claim: the challenger hangs off the pill it attacks by the same rule a
// challenger hangs off a claim, and a challenge to that challenge hangs off
// its pill in turn, at any depth. Nothing is a midpoint; nothing floats.
//
// Pure, so it can be reasoned about (and tested) without React.
const GAP_X = 200;             // a claim's slot width
const PILL_SLOT_X = 120;       // a pill's slot width when it is a leaf
const TOP_Y = 60;
const ROW_CLAIM_TO_PILL = 90;  // claim centre -> pill centre below it
const ROW_PILL_TO_CLAIM = 95;  // pill centre -> claim centre below it
const ROW_PILL_TO_PILL = 70;   // pill centre -> the undercutting pill below it
const ROW_SAME_KIND = 170;     // claim -> claim; should not occur, kept as a safe fallback

export function layoutDebate(
  nodes: GraphNode[],
  edges: GraphEdge[],
  width: number
): { positionedNodes: GraphNode[]; relationGroups: RelationGroup[] } {
  if (nodes.length === 0) return { positionedNodes: [], relationGroups: [] };

  const claimById = new Map<string, GraphNode>();
  nodes.forEach((n) => claimById.set(n.id, { ...n }));

  // 1. Group edges into relations. Several edges sharing a relationId are ONE
  //    relation with several sources -- one pill, several incoming segments.
  const groups = new Map<string, RelationGroup>();
  for (const edge of edges) {
    if (!isSupportEdge(edge.type) && !isChallengeEdge(edge.type)) continue;
    const source = claimById.get(edge.source);
    if (!source || !claimById.has(edge.target)) continue;
    const key = edge.relationId && edge.relationId > 0 ? `r${edge.relationId}` : `e${edge.id}`;
    const existing = groups.get(key);
    if (!existing) {
      groups.set(key, {
        key,
        sources: [source],
        targetId: edge.target,
        type: edge.type,
        label: junctionLabel(edge),
        selectable: !!(edge.relationId && edge.relationId > 0),
        targetRelationKey:
          edge.targetRelationId && edge.targetRelationId > 0 ? `r${edge.targetRelationId}` : undefined,
        x: 0, y: 0, tx: 0, ty: 0,
        undercutForViewer: false,
      });
    } else if (!existing.sources.some((s) => s.id === source.id)) {
      existing.sources.push(source);
    }
  }
  // An undercut whose target pill is not drawn falls back to the claim.
  for (const g of groups.values()) {
    if (g.targetRelationKey && !groups.has(g.targetRelationKey)) g.targetRelationKey = undefined;
  }

  // 2. The layout tree. Ids are claim ids and relation keys.
  const children = new Map<string, string[]>();
  const parentOf = new Map<string, string>();
  const isPill = (id: string) => groups.has(id);
  const link = (parent: string, child: string) => {
    if (parentOf.has(child)) return;
    parentOf.set(child, parent);
    if (!children.has(parent)) children.set(parent, []);
    children.get(parent)!.push(child);
  };
  // Pills first, so every claim's parent is a pill (not another claim), which
  // keeps rows alternating. A claim that sources several relations hangs from
  // the first; the others still draw their lines to it.
  for (const g of groups.values()) {
    link(g.targetRelationKey ?? g.targetId, g.key);
  }
  for (const g of groups.values()) {
    for (const src of g.sources) link(g.key, src.id);
  }

  let rootId = nodes.find((n) => n.isRoot)?.id ?? "";
  if (!rootId) rootId = nodes.find((n) => !parentOf.has(n.id))?.id ?? nodes[0].id;

  // 3. Reachable set, by BFS from the root; depth per node. Unreachable claims
  //    are dropped, as before -- a floating row would say nothing true.
  const depth = new Map<string, number>();
  const order: string[] = [rootId];
  depth.set(rootId, 0);
  for (let i = 0; i < order.length; i++) {
    const id = order[i];
    for (const c of children.get(id) ?? []) {
      if (!depth.has(c)) {
        depth.set(c, depth.get(id)! + 1);
        order.push(c);
      }
    }
  }
  const kids = (id: string) => (children.get(id) ?? []).filter((c) => depth.has(c));

  // 4. Widths, then x. A pill is narrow when it is a leaf, but as wide as its
  //    children when it has them, like any node.
  const widthOf = new Map<string, number>();
  const computeWidth = (id: string): number => {
    if (widthOf.has(id)) return widthOf.get(id)!;
    const ks = kids(id);
    const own = isPill(id) ? PILL_SLOT_X : GAP_X;
    const w = ks.length === 0 ? own : Math.max(own, ks.reduce((sum, c) => sum + computeWidth(c), 0));
    widthOf.set(id, w);
    return w;
  };
  const total = computeWidth(rootId);

  // 5. Vertical spacing is LOCAL, not row-based: a pill sits a fixed distance
  //    below whatever it argues about, a claim a fixed distance below its pill.
  //    Global rows cannot alternate once an undercut adds a pill level in one
  //    branch and not another; local spacing keeps every pill exactly where a
  //    pill goes, and lets deep branches simply run further down.
  const gapTo = (parent: string, child: string) => {
    const p = isPill(parent), c = isPill(child);
    if (!p && c) return ROW_CLAIM_TO_PILL;   // claim -> the pill that argues about it
    if (p && !c) return ROW_PILL_TO_CLAIM;   // pill -> the claim that feeds it
    if (p && c) return ROW_PILL_TO_PILL;     // pill -> the pill that undercuts it
    return ROW_SAME_KIND;                    // claim -> claim never happens; be safe
  };

  const pos = new Map<string, { x: number; y: number }>();
  const place = (id: string, x0: number, y: number) => {
    const w = widthOf.get(id) ?? GAP_X;
    pos.set(id, { x: x0 + w / 2, y });
    let cx = x0;
    for (const c of kids(id)) {
      place(c, cx, y + gapTo(id, c));
      cx += widthOf.get(c) ?? GAP_X;
    }
  };
  place(rootId, (width - total) / 2, TOP_Y);

  // 6. Write positions back. Claims get x/y; pills get x/y and their line end.
  const positionedNodes: GraphNode[] = [];
  for (const id of order) {
    if (isPill(id)) continue;
    const n = claimById.get(id)!;
    const at = pos.get(id)!;
    n.x = at.x;
    n.y = at.y;
    positionedNodes.push(n);
  }
  const relationGroups: RelationGroup[] = [];
  for (const g of groups.values()) {
    const at = pos.get(g.key);
    if (!at) continue;  // unreachable
    const end = pos.get(g.targetRelationKey ?? g.targetId);
    if (!end) continue;
    g.x = at.x; g.y = at.y; g.tx = end.x; g.ty = end.y;
    // Only sources that were placed can be drawn.
    g.sources = g.sources.filter((src) => pos.has(src.id)).map((src) => claimById.get(src.id)!);
    relationGroups.push(g);
  }
  // 7. Undercut for the viewer: a challenge aimed at this pill whose
  //    challenger the viewer holds TRUE. Statuses on nodes are already the
  //    viewer's, so this is the same test the status engine applies.
  const byKey = new Map(relationGroups.map((g) => [g.key, g] as const));
  for (const g of relationGroups) {
    if (!g.targetRelationKey || !isChallengeEdge(g.type)) continue;
    const target = byKey.get(g.targetRelationKey);
    if (target && g.sources.some((src) => src.status === "TRUE_CLAIM")) target.undercutForViewer = true;
  }
  return { positionedNodes, relationGroups };
}

