import { API_BASE } from './config';

// Get auth token from localStorage
const getAuthToken = () => {
  return localStorage.getItem('token');
};

// Create headers with auth token
const createHeaders = () => {
  const token = getAuthToken();
  return {
    'Content-Type': 'application/json',
    ...(token && { 'Authorization': `Bearer ${token}` })
  };
};

// Handle API errors
const handleResponse = async (response) => {
  const data = await response.json();
  
  if (!response.ok) {
    // If token is invalid/expired, clear it and let the app handle the state change
    if (response.status === 403 || response.status === 401) {
      console.log('Authentication failed, clearing token...');
      localStorage.removeItem('token');
      throw new Error('Authentication failed');
    }
    throw new Error(data.message || `HTTP error! status: ${response.status}`);
  }
  
  return data;
};

export const friendsAPI = {
  // Search for users to add as friends
  async searchUsers(query) {
    const response = await fetch(`${API_BASE}/friends/search?query=${encodeURIComponent(query)}`, {
      headers: createHeaders()
    });
    return handleResponse(response);
  },

  // Get friends list and pending requests
  async getFriends() {
    const response = await fetch(`${API_BASE}/friends`, {
      headers: createHeaders()
    });
    return handleResponse(response);
  },

  // Send friend request
  async sendFriendRequest(userId) {
    const response = await fetch(`${API_BASE}/friends/request/${userId}`, {
      method: 'POST',
      headers: createHeaders()
    });
    return handleResponse(response);
  },

  // Accept friend request
  async acceptFriendRequest(requestId) {
    const response = await fetch(`${API_BASE}/friends/request/${requestId}/accept`, {
      method: 'PUT',
      headers: createHeaders()
    });
    return handleResponse(response);
  },

  // Decline friend request
  async declineFriendRequest(requestId) {
    const response = await fetch(`${API_BASE}/friends/request/${requestId}/decline`, {
      method: 'PUT',
      headers: createHeaders()
    });
    return handleResponse(response);
  },

  // Remove friend
  async removeFriend(friendshipId) {
    const response = await fetch(`${API_BASE}/friends/friendship/${friendshipId}`, {
      method: 'DELETE',
      headers: createHeaders()
    });
    return handleResponse(response);
  },

  // Get chat history with a friend
  async getChatHistory(friendId, page = 1, limit = 50) {
    const response = await fetch(`${API_BASE}/friends/${friendId}/messages?page=${page}&limit=${limit}`, {
      headers: createHeaders()
    });
    return handleResponse(response);
  },

  // Send message to friend
  async sendMessage(friendId, content, messageType = 'text', roomInvite = null) {
    const response = await fetch(`${API_BASE}/friends/${friendId}/messages`, {
      method: 'POST',
      headers: createHeaders(),
      body: JSON.stringify({
        content,
        messageType,
        ...(roomInvite && { roomInvite })
      })
    });
    return handleResponse(response);
  },

  // Send room invitation to friend
  async sendRoomInvitation(friendId, roomId) {
    const response = await fetch(`${API_BASE}/friends/${friendId}/invite/${roomId}`, {
      method: 'POST',
      headers: createHeaders()
    });
    return handleResponse(response);
  }
};
