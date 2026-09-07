// graphLayout.test.ts -- drives layoutDebate with a small debate and checks
// the one rule holds: a node's children are the things that point at it.
//
// Run with `npm run test:layout`. No test runner is installed in this
// package; esbuild (already here for Vite) bundles this file for Node and
// Node runs it. Exit code is the verdict.
import { layoutDebate } from "./graphLayout";

const nodes = [
  { id: "1", text: "Speed limits save lives", isRoot: true, status: "UNDETERMINED" },
  { id: "2", text: "Crash deaths fell 23%", status: "TRUE_CLAIM" },
  { id: "3", text: "WHO recommends 30 km/h", status: "TRUE_CLAIM" },
  { id: "4", text: "Boroughs were getting safer anyway", status: "TRUE_CLAIM" },      // undercuts r10
  { id: "5", text: "Trend was flat elsewhere", status: "UNDETERMINED" },              // undercuts r20
  { id: "6", text: "WHO's figure is for mixed streets", status: "UNDETERMINED" },     // rebuts claim 3
  { id: "7", text: "Unreachable stray", status: "UNDETERMINED" },
];
const edges = [
  { id: "e1", source: "2", target: "1", type: "SUPPORTS", basis: "EVIDENTIAL", relationId: 10 },
  { id: "e2", source: "3", target: "1", type: "SUPPORTS", basis: "AUTHORITY", relationId: 11 },
  // undercut: stored beside the claim the argument was for (1), aimed at relation 10
  { id: "e3", source: "4", target: "1", type: "CHALLENGE", basis: "DEFINITIONAL", relationId: 20, targetRelationId: 10 },
  // undercut of the undercut: aimed at relation 20
  { id: "e4", source: "5", target: "1", type: "CHALLENGE", basis: "INFERENTIAL", relationId: 21, targetRelationId: 20 },
  // plain rebuttal of claim 3
  { id: "e5", source: "6", target: "3", type: "CHALLENGE", basis: "EVIDENTIAL", relationId: 30 },
];

const { positionedNodes, relationGroups } = layoutDebate(nodes as any, edges as any, 1200);
const claim = (id: string) => positionedNodes.find((n) => n.id === id)!;
const pill = (key: string) => relationGroups.find((g) => g.key === key)!;

let failures = 0;
const check = (cond: boolean, what: string) => { console.log((cond ? "ok   " : "FAIL ") + what); if (!cond) failures++; };
const near = (a: number, b: number) => Math.abs(a - b) < 0.01;

check(positionedNodes.map((n) => n.id).sort().join() === "1,2,3,4,5,6", "reachable claims placed, stray dropped");
check(relationGroups.length === 5, "five relations drawn");

// rows alternate: pill below the claim it argues for, source claim below the pill
check(pill("r10").y > claim("1").y && claim("2").y > pill("r10").y, "claim -> pill -> claim rows");
check(near(pill("r10").tx, claim("1").x) && near(pill("r10").ty, claim("1").y), "support pill's line ends at the claim centre");

// the undercut hangs off the pill it attacks, by the same rule
const u = pill("r20");
check(near(u.tx, pill("r10").x) && near(u.ty, pill("r10").y), "undercut's line ends at the attacked pill's centre");
check(u.y > pill("r10").y, "undercut pill sits in the row below its target pill");
// r20 has two children -- the challenger and the nested undercut -- so it
// centres over both, like any node over its subtree.
check(claim("4").y > u.y, "challenger hangs below its pill");
check(near(u.x, (claim("4").x + pill("r21").x) / 2), "the pill centres over its two children");

// nested: the undercut of the undercut hangs off r20
const u2 = pill("r21");
check(near(u2.tx, u.x) && near(u2.ty, u.y), "second-order undercut ends at the first undercut's pill");
check(claim("5").y > u2.y && near(claim("5").x, u2.x), "its challenger hangs below it, x-aligned");

// the rebuttal is drawn by the very same rule
const r = pill("r30");
check(near(r.tx, claim("3").x) && near(r.ty, claim("3").y), "rebuttal's line ends at the claim");
check(claim("6").y > r.y && near(claim("6").x, r.x), "rebutting claim hangs below its pill");

// undercut-for-viewer: 4 is TRUE for the viewer, so r10 is undercut; 5 is not TRUE, so r20 is not
check(pill("r10").undercutForViewer === true, "r10 marked undercut for the viewer");
check(pill("r20").undercutForViewer === false, "r20 not marked (its challenger is undetermined)");
check(pill("r11").undercutForViewer === false, "r11 untouched");

// the invariant that matters: every pill sits strictly between what it argues
// about (above) and what feeds it (below)
check(relationGroups.every((g) => g.y > g.ty && g.sources.every((src) => (src.y ?? 0) > g.y)),
      "every pill sits between its target and its sources");
// local spacing: a pill is a fixed distance below what it argues about, and its
// sources a fixed distance below it -- the same numbers everywhere
check(relationGroups.every((g) => near(g.y - g.ty, g.targetRelationKey ? 70 : 90)),
      "every pill hangs a fixed distance below its target (90 under a claim, 70 under a pill)");
check(relationGroups.every((g) => g.sources.every((src) => near((src.y ?? 0) - g.y, 95))),
      "every source claim hangs 95 below its pill");

console.log(failures === 0 ? "ALL CHECKS PASSED" : `${failures} CHECK(S) FAILED`);
process.exit(failures === 0 ? 0 : 1);
