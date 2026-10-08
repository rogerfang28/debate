import { createContext } from "react";

// Which page region a component is drawn inside ("inspector", "main",
// "chat", "page"), or null outside any. The server marks the regions
// (data-region); ContainerComponent provides this so a component deep inside
// one -- the docked chat in the chat region -- can take that region's
// placement without the server saying anything more.
export const RegionContext = createContext<string | null>(null);

// localStorage, for the reader's own view state (folds, the Graph | Text
// toggle). Storage can be unavailable; the state then lasts for the page.
export function readStored(key: string, fallback: string): string {
  try {
    return localStorage.getItem(key) ?? fallback;
  } catch {
    return fallback;
  }
}

export function writeStored(key: string, value: string): void {
  try {
    localStorage.setItem(key, value);
  } catch {
    // The state still changes for this page.
  }
}
