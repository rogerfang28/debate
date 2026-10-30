// connectionCards.ts -- what the selected claim is connected to, how to walk
// the map from it, and the cards that point at its connections.
//
// The first two are pure (relation groups in, ids out). The third writes plain
// DOM into a host div that React renders empty and never touches again: the
// cards follow the camera on every zoom event and animation frame, and doing
// that through React state would re-render every node of the graph per frame.

import type { GraphNode, RelationGroup } from "./graphLayout";
import { isChallengeEdge } from "./graphLayout";

// ── Connections ───────────────────────────────────────────────────────
//
// Claims are nodes; relations are pills. A claim is connected to the claims
// on the OTHER side of every relation it is part of, through the pill:
//   - a relation it is a source of: the claim that relation is about (its
//     parent). For an undercut -- a challenge aimed at an ARGUMENT -- that is
//     the claim the argument is for, which is what the group's targetId names.
//   - a relation about it: that relation's sources (its supports and
//     challenges), and the challengers of any ARGUMENT for it (undercuts whose
//     targetId is this claim).
// Co-sources of one relation are on the same side, so they are not connected.
//
// A PILL can be the focus too (its key, `r<id>`). It is connected to the
// claim it is about, to its sources, and to the sources of any pill that
// undercuts it.

export type LinkKind =
  | "parent"      // this claim supports it
  | "challenged"  // this claim challenges it
  | "undercutOf"  // this claim challenges an argument for it
  | "support"     // it supports this claim
  | "challenge"   // it challenges this claim
  | "undercut"    // it challenges an argument for this claim
  | "undercutter"; // it challenges this argument (a pill is the focus)

export interface Connection {
  id: string;
  kind: LinkKind;
}

export interface Focus {
  connections: Connection[];
  connectedIds: Set<string>;
  litGroups: Set<string>;   // pills on a lit path
  litLines: Set<string>;    // `${groupKey}|${sourceId}` and `${groupKey}|out`
}

export const lineKey = (groupKey: string, end: string) => `${groupKey}|${end}`;

export function focusOf(selectedId: string | null, groups: RelationGroup[]): Focus | null {
  if (!selectedId) return null;
  const connections: Connection[] = [];
  const connectedIds = new Set<string>();
  const litGroups = new Set<string>();
  const litLines = new Set<string>();
  const add = (id: string, kind: LinkKind) => {
    if (id === selectedId || connectedIds.has(id)) return;
    connectedIds.add(id);
    connections.push({ id, kind });
  };

  // The focus is a pill: the claim it is about, then its sources, then the
  // challengers of the pill itself.
  const pill = groups.find((g) => g.key === selectedId);
  if (pill) {
    const challenge = isChallengeEdge(pill.type);
    add(pill.targetId, challenge ? (pill.targetRelationKey ? "undercutOf" : "challenged") : "parent");
    litGroups.add(pill.key);
    litLines.add(lineKey(pill.key, "out"));
    for (const src of pill.sources) {
      add(src.id, challenge ? "challenge" : "support");
      litLines.add(lineKey(pill.key, src.id));
    }
    for (const g of groups) {
      if (g.targetRelationKey !== pill.key) continue;
      for (const src of g.sources) {
        add(src.id, "undercutter");
        litLines.add(lineKey(g.key, src.id));
      }
      litGroups.add(g.key);
      litLines.add(lineKey(g.key, "out"));
    }
    return { connections, connectedIds, litGroups, litLines };
  }

  // Upward first, so a claim's parent is its first connection.
  for (const g of groups) {
    if (!g.sources.some((s) => s.id === selectedId) || g.targetId === selectedId) continue;
    const challenge = isChallengeEdge(g.type);
    add(g.targetId, challenge ? (g.targetRelationKey ? "undercutOf" : "challenged") : "parent");
    litGroups.add(g.key);
    litLines.add(lineKey(g.key, selectedId));
    litLines.add(lineKey(g.key, "out"));
  }
  for (const g of groups) {
    if (g.targetId !== selectedId) continue;
    const challenge = isChallengeEdge(g.type);
    for (const src of g.sources) {
      add(src.id, challenge ? (g.targetRelationKey ? "undercut" : "challenge") : "support");
      litLines.add(lineKey(g.key, src.id));
    }
    litGroups.add(g.key);
    litLines.add(lineKey(g.key, "out"));
  }
  return { connections, connectedIds, litGroups, litLines };
}

// ── Walking the map ───────────────────────────────────────────────────
//
// Up is the claim this one hangs from in the layout: the target of the FIRST
// relation it is a source of (layoutDebate hangs a claim from its first
// relation the same way). Down and sideways are that claim's children --
// everything that argues about it, directly or by undercutting an argument
// for it -- in left-to-right order on the map.

export interface Navigation {
  parent?: string;
  firstChild?: string;
  prevSibling?: string;
  nextSibling?: string;
}

export function navigationFor(
  id: string,
  groups: RelationGroup[],
  nodeById: Map<string, GraphNode>
): Navigation {
  const parentOf = (claim: string) =>
    groups.find((g) => g.targetId !== claim && g.sources.some((s) => s.id === claim))?.targetId;
  const childrenOf = (claim: string) => {
    const seen = new Set<string>();
    for (const g of groups) {
      if (g.targetId !== claim) continue;
      for (const s of g.sources) if (s.id !== claim && nodeById.has(s.id)) seen.add(s.id);
    }
    return [...seen].sort((a, b) => {
      const na = nodeById.get(a)!, nb = nodeById.get(b)!;
      return (na.x ?? 0) - (nb.x ?? 0) || (na.y ?? 0) - (nb.y ?? 0);
    });
  };
  const pill = groups.find((g) => g.key === id);
  if (pill) {
    const kids = pill.sources
      .filter((s) => nodeById.has(s.id))
      .sort((a, b) => (nodeById.get(a.id)!.x ?? 0) - (nodeById.get(b.id)!.x ?? 0));
    return { parent: pill.targetId, firstChild: kids[0]?.id };
  }
  const parent = parentOf(id);
  const kids = childrenOf(id);
  const nav: Navigation = { parent, firstChild: kids[0] };
  if (parent) {
    const sibs = childrenOf(parent);
    const i = sibs.indexOf(id);
    if (i > 0) nav.prevSibling = sibs[i - 1];
    if (i >= 0 && i < sibs.length - 1) nav.nextSibling = sibs[i + 1];
  }
  return nav;
}

// ── Connection cards ──────────────────────────────────────────────────

// A card is coloured by its claim's STATUS (the graph passes the colour the
// node's status line uses): green true, red false, grey undetermined. The
// kind of link is in the label, not the colour.
const NEUTRAL_COLOR = "var(--text-3)";

const KIND_LABEL: Record<LinkKind, string> = {
  parent: "the claim this supports",
  challenged: "the claim this challenges",
  undercutOf: "the claim whose argument this challenges",
  support: "support",
  challenge: "challenge",
  undercut: "challenges an argument for this",
  undercutter: "challenges this argument",
};

export interface CardTarget {
  id: string;
  kind: LinkKind;
  color: string;  // the claim's status colour, a CSS colour or var()
  who: string;
  text: string;
  x: number;  // the connected claim's centre, in pane pixels
  y: number;
}

type Side = "left" | "right" | "top" | "bottom";
// Cards that share a run along one edge. In a pane too narrow for two cards
// side by side, the left and right edges are the same column, so everything
// goes into one.
type Run = Side | "column";

interface Placed {
  key: string;
  run: Run;
  el: HTMLElement;
  x: number;
  y: number;
  w: number;
  h: number;
}

const INSET = 34;      // where the line is cut, inside the pane edge
const MARGIN = 8;      // a card stays this far inside the pane
const GAP = 6;         // between two cards
const MAX_PER_SIDE = 6;  // more than this on one edge: the nearest few and "+N more"
const CARD_W = 300;      // index.css .graph-card
const EST_CARD_H = 112;  // a three-line sentence; only used to budget a run

const arrowFor = (dx: number, dy: number) =>
  Math.abs(dx) > Math.abs(dy) ? (dx > 0 ? "→" : "←") : dy > 0 ? "↓" : "↑";

// Elements are reused by key so a card under the pointer survives a camera
// move: rebuilding it every frame would drop a click that spans a flight.
function cardElement(host: HTMLElement, key: string, onPick: (id: string) => void): HTMLElement {
  let el = host.querySelector<HTMLElement>(`:scope > [data-card="${CSS.escape(key)}"]`);
  if (el) return el;
  el = document.createElement("div");
  el.dataset.card = key;
  el.setAttribute("role", "button");
  el.className = "graph-card";
  el.innerHTML =
    '<span class="graph-card-dot"></span>' +
    '<span class="graph-card-top"><span class="graph-card-badge">Parent</span><span class="graph-card-who"></span><span class="graph-card-kind"></span></span>' +
    '<span class="graph-card-arrow"></span>' +
    '<span class="graph-card-text"></span>';
  el.addEventListener("click", (e) => {
    e.stopPropagation();
    const id = el!.dataset.target;
    if (id) onPick(id);
  });
  host.appendChild(el);
  return el;
}

// The claim the selection hangs from (it supports it, challenges it, or
// challenges an argument for it) is the one way back up the map, so its card
// is marked: a "Parent" badge and an accent ring.
const PARENT_KINDS = new Set<LinkKind>(["parent", "challenged", "undercutOf"]);

// The two things a card says, on two channels so they never compete: the
// claim's STATUS is the dot (green true, red false, grey undetermined), and
// the RELATION to the selection is the border and the label (orange dashed
// for a challenge, as challenge lines are on the map; plain for a support).
const CHALLENGE_KINDS = new Set<LinkKind>(["challenged", "undercutOf", "challenge", "undercut", "undercutter"]);

function fill(el: HTMLElement, color: string, who: string, kind: string, arrow: string, text: string, target: string,
              parent = false, challenge = false) {
  if (el.style.getPropertyValue("--card-color") !== color) el.style.setProperty("--card-color", color);
  el.classList.toggle("graph-card-parent", parent);
  el.classList.toggle("graph-card-challenge", challenge);
  el.dataset.target = target;
  const set = (sel: string, value: string) => {
    const part = el.querySelector<HTMLElement>(sel)!;
    if (part.textContent !== value) part.textContent = value;
  };
  set(".graph-card-who", who);
  set(".graph-card-kind", kind);
  set(".graph-card-arrow", arrow);
  set(".graph-card-text", text);
  el.title = text;
}

// Draw one card per connection, at the point where the straight line from
// the focus through its claim leaves the pane (so a claim that is in view
// gets a card at the edge beyond it, pointing back at it); then keep every
// card fully inside the pane and push overlapping ones apart.
export function placeConnectionCards(
  host: HTMLElement,
  focus: { x: number; y: number } | null,
  targets: CardTarget[],
  pane: { width: number; height: number },
  onPick: (id: string) => void
): void {
  const keep = new Set<string>();
  const placed: Placed[] = [];

  if (focus && pane.width > 0 && pane.height > 0) {
    const cardW = Math.min(CARD_W, pane.width - 2 * MARGIN);
    const narrow = pane.width < 2 * cardW + 2 * MARGIN + GAP;
    // How many cards a run holds: MAX_PER_SIDE, or fewer if the pane is small.
    const fitAlong = (len: number, size: number) => Math.max(1, Math.floor((len - 2 * MARGIN + GAP) / (size + GAP)));
    const capacity = (run: Run) =>
      Math.min(MAX_PER_SIDE, run === "top" || run === "bottom" ? fitAlong(pane.width, cardW) : fitAlong(pane.height, EST_CARD_H));

    // 1. The exit point and the side it is on, per connection.
    const bySide = new Map<Run, { t: CardTarget; x: number; y: number; dx: number; dy: number; dist: number }[]>();
    for (const t of targets) {
      const dx = t.x - focus.x, dy = t.y - focus.y;
      let s = Infinity;
      let side: Side = "right";
      const cut = (v: number, sd: Side) => { if (v >= 0 && v < s) { s = v; side = sd; } };
      if (dx > 0) cut((pane.width - INSET - focus.x) / dx, "right");
      if (dx < 0) cut((INSET - focus.x) / dx, "left");
      if (dy > 0) cut((pane.height - INSET - focus.y) / dy, "bottom");
      if (dy < 0) cut((INSET - focus.y) / dy, "top");
      if (!isFinite(s)) s = 0;
      const entry = { t, x: focus.x + dx * s, y: focus.y + dy * s, dx, dy, dist: Math.hypot(dx, dy) };
      const run: Run = narrow ? "column" : side;
      if (!bySide.has(run)) bySide.set(run, []);
      bySide.get(run)!.push(entry);
    }

    // 2. A crowded run shows its nearest few and one "+N more" card.
    for (const [run, list] of bySide) {
      // The parent first, so a crowded edge never folds it into "+N more";
      // then nearest first.
      const up = (c: { t: CardTarget }) => (PARENT_KINDS.has(c.t.kind) ? 1 : 0);
      list.sort((a, b) => up(b) - up(a) || a.dist - b.dist);
      const cap = capacity(run);
      const shown = list.length > cap ? list.slice(0, cap - 1) : list;
      for (const c of shown) {
        const key = `c${c.t.id}`;
        const el = cardElement(host, key, onPick);
        fill(el, c.t.color, c.t.who, KIND_LABEL[c.t.kind], arrowFor(c.dx, c.dy), c.t.text, c.t.id,
             PARENT_KINDS.has(c.t.kind), CHALLENGE_KINDS.has(c.t.kind));
        keep.add(key);
        placed.push({ key, run, el, x: c.x, y: c.y, w: 0, h: 0 });
      }
      if (shown.length < list.length) {
        const rest = list.slice(shown.length);
        const key = `more-${run}`;
        const el = cardElement(host, key, onPick);
        const n = rest.length;
        fill(
          el, NEUTRAL_COLOR, `+${n} more`, "connections this way", arrowFor(rest[0].dx, rest[0].dy),
          `Fly to the nearest: ${rest[0].t.text}`, rest[0].t.id
        );
        keep.add(key);
        const ax = rest.reduce((sum, c) => sum + c.x, 0) / n;
        const ay = rest.reduce((sum, c) => sum + c.y, 0) / n;
        placed.push({ key, run, el, x: ax, y: ay, w: 0, h: 0 });
      }
    }
  }

  // Cards that no longer point anywhere go.
  for (const el of Array.from(host.children) as HTMLElement[]) {
    if (!keep.has(el.dataset.card ?? "")) el.remove();
  }
  if (placed.length === 0) return;

  // 3. Measure, clamp inside the pane, separate. Cards are centred on (x, y).
  for (const q of placed) { q.w = q.el.offsetWidth; q.h = q.el.offsetHeight; }
  const clamp = (q: Placed) => {
    q.x = Math.max(MARGIN + q.w / 2, Math.min(pane.width - MARGIN - q.w / 2, q.x));
    q.y = Math.max(MARGIN + q.h / 2, Math.min(pane.height - MARGIN - q.h / 2, q.y));
  };
  placed.forEach(clamp);
  // Cards on one edge sit in one column (left/right) or row (top/bottom):
  // stack them in order along it, then pull the run back inside the pane.
  // Pairwise shoving alone stalls against the clamp and leaves overlaps.
  const stack = (list: Placed[], axis: "x" | "y") => {
    if (list.length < 2) return;
    const size = (q: Placed) => (axis === "y" ? q.h : q.w);
    const hi = (axis === "y" ? pane.height : pane.width) - MARGIN;
    list.sort((a, b) => a[axis] - b[axis]);
    for (let i = 1; i < list.length; i++) {
      const min = list[i - 1][axis] + (size(list[i - 1]) + size(list[i])) / 2 + GAP;
      if (list[i][axis] < min) list[i][axis] = min;
    }
    const last = list[list.length - 1];
    last[axis] = Math.min(last[axis], hi - size(last) / 2);
    for (let i = list.length - 2; i >= 0; i--) {
      const max = list[i + 1][axis] - (size(list[i + 1]) + size(list[i])) / 2 - GAP;
      if (list[i][axis] > max) list[i][axis] = max;
    }
    list.forEach(clamp);
  };
  for (const run of ["left", "right", "top", "bottom", "column"] as Run[]) {
    stack(placed.filter((q) => q.run === run), run === "top" || run === "bottom" ? "x" : "y");
  }
  // Whatever still collides -- cards from two edges meeting at a corner.
  for (let pass = 0; pass < 16; pass++) {
    let moved = false;
    for (let i = 0; i < placed.length; i++) {
      for (let j = i + 1; j < placed.length; j++) {
        const a = placed[i], b = placed[j];
        const ox = (a.w + b.w) / 2 + GAP - Math.abs(a.x - b.x);
        const oy = (a.h + b.h) / 2 + GAP - Math.abs(a.y - b.y);
        if (ox <= 0 || oy <= 0) continue;
        moved = true;
        // The smaller shove: vertical on a left/right edge, horizontal on top/bottom.
        if (oy < ox) {
          const s = a.y <= b.y ? 1 : -1;
          a.y -= (s * oy) / 2; b.y += (s * oy) / 2;
        } else {
          const s = a.x <= b.x ? 1 : -1;
          a.x -= (s * ox) / 2; b.x += (s * ox) / 2;
        }
        clamp(a); clamp(b);
      }
    }
    if (!moved) break;
  }
  for (const q of placed) {
    q.el.style.left = `${q.x - q.w / 2}px`;
    q.el.style.top = `${q.y - q.h / 2}px`;
  }
}
