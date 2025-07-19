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
    throw new Error(data.message || `HTTP error! status: ${response.status}`);
  }
  
  return data;
};

export const roomAPI = {
  // Get all rooms for the current user
  async getRooms() {
    const response = await fetch(`${API_BASE}/rooms`, {
      headers: createHeaders()
    });
    return handleResponse(response);
  },

  // Get specific room details
  async getRoom(roomId) {
    const response = await fetch(`${API_BASE}/rooms/${roomId}`, {
      headers: createHeaders()
    });
    return handleResponse(response);
  },

  // Create a new room
  async createRoom(roomData) {
    const response = await fetch(`${API_BASE}/rooms`, {
      method: 'POST',
      headers: createHeaders(),
      body: JSON.stringify(roomData)
    });
    return handleResponse(response);
  },

  // Update room settings
  async updateRoom(roomId, updates) {
    const response = await fetch(`${API_BASE}/rooms/${roomId}`, {
      method: 'PUT',
      headers: createHeaders(),
      body: JSON.stringify(updates)
    });
    return handleResponse(response);
  },

  // Delete room
  async deleteRoom(roomId) {
    const response = await fetch(`${API_BASE}/rooms/${roomId}`, {
      method: 'DELETE',
      headers: createHeaders()
    });
    return handleResponse(response);
  },

  // Join room by invite code
  async joinRoomByCode(inviteCode) {
    const response = await fetch(`${API_BASE}/rooms/join/${inviteCode}`, {
      method: 'POST',
      headers: createHeaders()
    });
    return handleResponse(response);
  },

  // Invite user to room
  async inviteToRoom(roomId, email) {
    const response = await fetch(`${API_BASE}/rooms/${roomId}/invite`, {
      method: 'POST',
      headers: createHeaders(),
      body: JSON.stringify({ email })
    });
    return handleResponse(response);
  },

  // Remove member from room
  async removeMember(roomId, userId) {
    const response = await fetch(`${API_BASE}/rooms/${roomId}/members/${userId}`, {
      method: 'DELETE',
      headers: createHeaders()
    });
    return handleResponse(response);
  },

  // Update member role
  async updateMemberRole(roomId, userId, role) {
    const response = await fetch(`${API_BASE}/rooms/${roomId}/members/${userId}/role`, {
      method: 'PUT',
      headers: createHeaders(),
      body: JSON.stringify({ role })
    });
    return handleResponse(response);
  },

  // Leave room
  async leaveRoom(roomId) {
    const response = await fetch(`${API_BASE}/rooms/${roomId}/leave`, {
      method: 'POST',
      headers: createHeaders()
    });
    return handleResponse(response);
  },

  // Generate new invite code
  async generateInviteCode(roomId) {
    const response = await fetch(`${API_BASE}/rooms/${roomId}/invite-code`, {
      method: 'POST',
      headers: createHeaders()
    });
    return handleResponse(response);
  },

  // Get room's graph data
  async getRoomGraph(roomId) {
    const response = await fetch(`${API_BASE}/rooms/${roomId}/graph`, {
      headers: createHeaders()
    });
    return handleResponse(response);
  }
};
