/**
 * Simple user authentication utilities using cookies
 * This will be easy to extend into a full login system later
 */

/**
 * Get the current user from cookies
 * Returns "guest" if no user is set
 */
export function getUser(): string {
  const cookies = document.cookie.split(';');
  for (const cookie of cookies) {
    const [key, value] = cookie.trim().split('=');
    if (key === 'user') {
      return value;
    }
  }
  return 'guest';
}

/**
 * Set the user cookie
 * @param username - The username to store
 * @param maxAgeDays - How long to keep the cookie (default: 30 days)
 */
export function setUser(username: string, maxAgeDays: number = 30): void {
  const maxAge = maxAgeDays * 24 * 60 * 60; // Convert days to seconds
  document.cookie = `user=${username}; path=/; max-age=${maxAge}; SameSite=Lax`;
  console.log('[Auth] User set:', username);
}

/**
 * Clear the user cookie (logout)
 */
export function clearUser(): void {
  document.cookie = 'user=; path=/; max-age=0';
  console.log('[Auth] User cleared');
}

/**
 * Check if a user is logged in (not guest)
 */
export function isLoggedIn(): boolean {
  return getUser() !== 'guest';
}

/**
 * Silent per-visitor identity.
 *
 * A first-time reader lands on the home page; there is no login page. The
 * client makes up a name like "visitor-a1b2c3d4", keeps it in localStorage so
 * the same reader comes back as the same visitor, and sends it with every
 * message (postClientMessageToCPP). When a request has no valid session the
 * server signs it in as that visitor. Each visitor is a separate user on the
 * server -- their own open debate, selected claim, and tab -- and the server
 * lets a visitor read everything and change nothing.
 */
const VISITOR_KEY = 'debate.visitorName';
const VISITOR_SHAPE = /^visitor-[0-9a-f]{8}$/;

function makeVisitorName(): string {
  const bytes = new Uint8Array(4);
  crypto.getRandomValues(bytes);
  return 'visitor-' + Array.from(bytes, (b) => b.toString(16).padStart(2, '0')).join('');
}

/** The stored visitor name, or a new one (stored when storage allows). */
export function getOrCreateVisitorName(): string {
  try {
    const stored = localStorage.getItem(VISITOR_KEY);
    if (stored && VISITOR_SHAPE.test(stored)) return stored;
  } catch {
    // Storage blocked: a fresh name per load still lets the reader read.
  }
  const name = makeVisitorName();
  try {
    localStorage.setItem(VISITOR_KEY, name);
  } catch {
    // ignore
  }
  return name;
}
