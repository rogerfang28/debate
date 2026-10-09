// The reader's view state that lives only in this page: where each scroller
// was, and what they have typed but not sent.
//
// Every click, and the 2-second poll while an AI job runs, replaces the whole
// page with a fresh server tree. With stable ids React keeps the DOM nodes,
// so most of this survives on its own -- but not when a subtree is rebuilt
// (Debate and Brief have different roots, so switching remounts the chat),
// and not when content above a scroller grows. This is the backstop.

// ── Scroll positions ─────────────────────────────────────────────────

export type ScrollSnapshot = Map<string, number>;

// A scroller's identity across renders: its id, or the nearest ancestor's id
// plus its own class (the regions' inner scrollers -- .region-scroll,
// .docked-turns -- have no id of their own, but sit directly under one).
function scrollKey(el: Element): string | null {
  if (el.id) return `#${el.id}`;
  const owner = el.parentElement?.closest("[id]");
  if (!owner || !el.className || typeof el.className !== "string") return null;
  return `#${owner.id}>.${el.className.trim().split(/\s+/).join(".")}`;
}

function scrollers(): Element[] {
  const root = document.querySelector(".page-root");
  const out: Element[] = [];
  if (document.scrollingElement) out.push(document.scrollingElement);
  if (root) {
    for (const el of root.querySelectorAll("*")) {
      if (el.scrollHeight > el.clientHeight + 1) out.push(el);
    }
  }
  return out;
}

/** Where every scrolled element is, by key. Taken just before a new page. */
export function snapshotScroll(): ScrollSnapshot {
  const snap: ScrollSnapshot = new Map();
  for (const el of scrollers()) {
    if (el.scrollTop <= 0) continue;
    const key = el === document.scrollingElement ? "document" : scrollKey(el);
    if (key && !snap.has(key)) snap.set(key, el.scrollTop);
  }
  return snap;
}

/** Puts each scroller back, unless its content is now too short for it. */
export function restoreScroll(snap: ScrollSnapshot): void {
  if (snap.size === 0) return;
  for (const el of scrollers()) {
    const key = el === document.scrollingElement ? "document" : scrollKey(el);
    if (!key) continue;
    const top = snap.get(key);
    if (top === undefined || el.scrollTop === top) continue;
    // Shrunk below where the reader was: leave it where the browser put it.
    if (el.scrollHeight - el.clientHeight < top) continue;
    el.scrollTop = top;
  }
}

// ── Drafts ───────────────────────────────────────────────────────────
//
// Text typed into a server-rendered box and not yet sent. Kept per box id,
// re-applied after every render, and dropped when the box's own send goes
// out. Only boxes listed here: a draft resurfacing in, say, a reopened
// add-claim form would be a surprise, not a help.
//
// box id -> the events that SEND it (component id, or "id:eventName").
const DRAFT_BOXES: Record<string, string[]> = {
  chatQuestionInput: ["askChatButton", "chatQuestionInput:onSubmit"],
};

const drafts = new Map<string, string>();

export function keepsDraft(id: string | undefined): boolean {
  return !!id && id in DRAFT_BOXES;
}

export function setDraft(id: string, text: string): void {
  if (text) drafts.set(id, text);
  else drafts.delete(id);
}

export function getDraft(id: string): string | undefined {
  return drafts.get(id);
}

/** Called for every event sent: drops the drafts that event sends. */
export function draftsSentBy(componentId: string, eventName: string): void {
  for (const [box, senders] of Object.entries(DRAFT_BOXES)) {
    if (senders.includes(componentId) || senders.includes(`${componentId}:${eventName}`)) {
      drafts.delete(box);
    }
  }
}
