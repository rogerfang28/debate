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
