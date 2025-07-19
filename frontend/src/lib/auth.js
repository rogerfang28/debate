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

// Check if token is expired
export function isTokenExpired() {
  const user = getCurrentUser();
  if (!user || !user.exp) return true;
  
  // exp is in seconds, Date.now() is in milliseconds
  return Date.now() >= user.exp * 1000;
}
