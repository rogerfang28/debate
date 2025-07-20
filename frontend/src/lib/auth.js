// Utility function to decode JWT token and get user info
export function getCurrentUser() {
  const token = localStorage.getItem('token');
  if (!token) return null;
  
  try {
    // JWT tokens have 3 parts separated by dots: header.payload.signature
    const payload = token.split('.')[1];
    
    // Decode base64 payload
    const decodedPayload = atob(payload);
    
    // Parse JSON
    const userInfo = JSON.parse(decodedPayload);
    
    return {
      userId: userInfo.userId,
      email: userInfo.email,
      username: userInfo.username,
      createdAt: userInfo.createdAt,
      exp: userInfo.exp
    };
  } catch (error) {
    console.error('Error decoding token:', error);
    return null;
  }
}

// Set token and reconnect socket
export function setAuthToken(token) {
  localStorage.setItem('token', token);
  
  // Reconnect socket with new token
  import('./socket.js').then(({ reconnectSocket }) => {
    reconnectSocket();
  });
}

// Check if token is expired
export function isTokenExpired() {
  const user = getCurrentUser();
  if (!user || !user.exp) return true;
  
  // exp is in seconds, Date.now() is in milliseconds
  return Date.now() >= user.exp * 1000;
}

// Logout function to clear token
export function logout() {
  localStorage.removeItem('token');
  // Don't force redirect - let React handle the state change
}

// Check authentication status and logout if expired
export function checkAuthAndLogout() {
  if (isTokenExpired()) {
    console.log('Token expired, logging out...');
    logout();
    return false;
  }
  return true;
}
