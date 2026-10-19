// React keys for the children the server sends.
//
// Every click re-renders the whole page from a fresh server tree, so a
// child's key is what tells React "this is the same element as last time":
// keep its DOM node, its scroll, its state. The server gives every component
// an id built from what it shows (a claim id, a person, a fixed name), and
// that id is the key.
//
// A child without an id gets `<parent id>#<index>`: positional, but scoped to
// its parent, so it cannot collide with a sibling that does have an id, or
// with a fallback anywhere else. Two siblings with the same id would make
// React drop or merge one; the second gets `#<index>` appended.
//
// Either case is a regression on the server, so in development each one is
// reported in a console warning, once per id per session, batched per render.

const reported = new Set<string>();
let pendingMissing: string[] = [];
let pendingCollided: string[] = [];
let flushTimer: ReturnType<typeof setTimeout> | undefined;

function flush(): void {
  flushTimer = undefined;
  if (pendingMissing.length === 0 && pendingCollided.length === 0) return;
  console.warn(
    "[childKeys] components without a stable id -- React may remount or move them on re-render.",
    { missingId: pendingMissing, duplicateId: pendingCollided }
  );
  pendingMissing = [];
  pendingCollided = [];
}

function report(kind: "missing" | "collided", key: string): void {
  if (!import.meta.env.DEV) return;
  const tag = `${kind}:${key}`;
  if (reported.has(tag)) return;
  reported.add(tag);
  (kind === "missing" ? pendingMissing : pendingCollided).push(key);
  if (flushTimer === undefined) flushTimer = setTimeout(flush, 0);
}

/** One key per child, in order. `parentId` scopes the fallbacks. */
// eslint-disable-next-line @typescript-eslint/no-explicit-any
export function childKeys(parentId: string | undefined, children: readonly any[]): string[] {
  const scope = parentId || "anonymous";
  const seen = new Set<string>();
  return children.map((child, idx) => {
    let key: string = typeof child?.id === "string" ? child.id : "";
    if (!key) {
      key = `${scope}#${idx}`;
      report("missing", key);
    }
    if (seen.has(key)) {
      report("collided", `${key} (in ${scope})`);
      key = `${key}#${idx}`;
    }
    seen.add(key);
    return key;
  });
}
