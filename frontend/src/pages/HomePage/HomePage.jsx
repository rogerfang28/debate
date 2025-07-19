import { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import AuthForm from '../../components/AuthForm';
import { RoomCard } from '../../components/RoomCard';
import { CreateRoomModal } from '../../components/CreateRoomModal';
import { JoinRoomForm } from '../../components/JoinRoomForm';
import { ManageRoomModal } from '../../components/ManageRoomModal';
import { FriendsModal } from '../../components/FriendsModal';
import { ChatFloat } from '../../components/ChatFloat';
import { roomAPI } from '../../lib/roomAPI';
import { friendsAPI } from '../../lib/friendsAPI';
import { socket, reconnectSocket } from '../../lib/socket';

export default function HomePage() {
  const [token, setToken] = useState(localStorage.getItem('token'));
  const [rooms, setRooms] = useState([]);
  const [isLoading, setIsLoading] = useState(false);
  const [showCreateModal, setShowCreateModal] = useState(false);
  const [showManageModal, setShowManageModal] = useState(false);
  const [showFriendsModal, setShowFriendsModal] = useState(false);
  const [selectedRoom, setSelectedRoom] = useState(null);
  const [error, setError] = useState('');
  const [friendNotificationCount, setFriendNotificationCount] = useState(0);
  const navigate = useNavigate();

  useEffect(() => {
    if (token) {
      // Reconnect socket with new token
      reconnectSocket();
      loadRooms();
      loadFriendNotifications();
      setupSocketListeners();
    }
  }, [token]);

  const setupSocketListeners = () => {
    // Clean up existing listeners first
    socket.off('friend-request-received-homepage');
    socket.off('friend-request-accepted-homepage');

    socket.on('friend-request-received-homepage', () => {
      console.log('🏠 Friend request received in HomePage');
      setFriendNotificationCount(prev => prev + 1);
    });

    socket.on('friend-request-accepted-homepage', () => {
      console.log('🏠 Friend request accepted in HomePage');
      loadFriendNotifications(); // Refresh count
    });

    return () => {
      socket.off('friend-request-received-homepage');
      socket.off('friend-request-accepted-homepage');
    };
  };

  const loadFriendNotifications = async () => {
    try {
      const data = await friendsAPI.getFriends();
      setFriendNotificationCount(data.pending.received.length);
    } catch (error) {
      console.error('Failed to load friend notifications:', error);
    }
  };

  const loadRooms = async () => {
    setIsLoading(true);
    setError('');
    try {
      const response = await roomAPI.getRooms();
      setRooms(response.rooms);
    } catch (err) {
      console.error('Failed to load rooms:', err);
      setError('Failed to load rooms. Please try again.');
    } finally {
      setIsLoading(false);
    }
  };

  const handleCreateRoom = async (roomData) => {
    try {
      const response = await roomAPI.createRoom(roomData);
      setRooms(prev => [response.room, ...prev]);
      return response;
    } catch (err) {
      throw new Error(err.message || 'Failed to create room');
    }
  };

  const handleDeleteRoom = (deletedRoomId) => {
    // Remove the deleted room from the rooms list
    setRooms(prev => prev.filter(room => room._id !== deletedRoomId));
    setShowManageModal(false);
    setSelectedRoom(null);
  };

  const handleJoinRoom = async (inviteCode) => {
    try {
      const response = await roomAPI.joinRoomByCode(inviteCode);
      setRooms(prev => [response.room, ...prev]);
      return response;
    } catch (err) {
      throw new Error(err.message || 'Failed to join room');
    }
  };

  const handleEnterRoom = async (roomId) => {
    try {
      // Navigate to graph page with room ID
      navigate(`/graph/${roomId}`);
    } catch (err) {
      console.error('Failed to enter room:', err);
    }
  };

  const handleManageRoom = (room) => {
    // Set the room to manage and open a management modal/view
    setSelectedRoom(room);
    setShowManageModal(true);
  };

  const handleLogout = () => {
    localStorage.removeItem('token');
    setToken(null);
    setRooms([]);
  };

  if (!token) {
    return (
      <div className="app-container">
        <div className="app-navigation">
          <div className="nav-content">
            <div className="nav-brand">
              <span className="brand-icon">🗣️</span>
              <span className="brand-text">DebateGraph</span>
            </div>
            <div className="nav-actions">
              <button 
                onClick={() => navigate('/public-debates')}
                className="nav-button btn-tertiary"
              >
                🌍 Browse Public Debates
              </button>
            </div>
          </div>
        </div>
        
        <div className="app-main">
          <div className="container min-h-screen flex items-center justify-center">
            <AuthForm onAuthSuccess={setToken} />
          </div>
        </div>
      </div>
    );
  }

  return (
    <div className="app-container">
      {/* Navigation Header */}
      <div className="app-navigation">
        <div className="nav-content">
          <div className="nav-brand">
            <span className="brand-icon">🗣️</span>
            <span className="brand-text">DebateGraph</span>
          </div>
          
          <div className="nav-actions">
            <button 
              onClick={() => navigate('/public-debates')}
              className="nav-button btn-tertiary"
              style={{ marginRight: '12px' }}
            >
              🌍 Public Debates
            </button>
            <button 
              onClick={() => {
                setShowFriendsModal(true);
                setFriendNotificationCount(0); // Clear notification when opened
              }}
              className="nav-button btn-primary"
              style={{ marginRight: '12px', position: 'relative' }}
            >
              👥 Friends
              {friendNotificationCount > 0 && (
                <span style={{
                  position: 'absolute',
                  top: '-5px',
                  right: '-5px',
                  background: '#ef4444',
                  color: 'white',
                  borderRadius: '50%',
                  minWidth: '18px',
                  height: '18px',
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'center',
                  fontSize: '11px',
                  fontWeight: 'bold',
                  border: '2px solid white'
                }}>
                  {friendNotificationCount > 99 ? '99+' : friendNotificationCount}
                </span>
              )}
            </button>
            <button 
              onClick={() => navigate('/profile')}
              className="nav-button btn-primary"
              style={{ marginRight: '12px' }}
            >
              👤 Profile
            </button>
            <button 
              onClick={handleLogout}
              className="nav-button btn-secondary"
            >
              🚪 Logout
            </button>
          </div>
        </div>
      </div>

      {/* Main Content */}
      <div className="app-main">
        <div className="container py-8">
          <div className="text-center mb-8">
            <h1>Your Debate Rooms</h1>
            <p className="text-gray-300 text-lg">
              Create, join, and manage collaborative debate spaces
            </p>
          </div>

          {/* Join Room Section */}
          <JoinRoomForm onJoinRoom={handleJoinRoom} />

          {/* Error Display */}
          {error && (
            <div className="bg-red-900/50 border border-red-600/50 text-red-200 p-4 rounded-lg mb-6">
              {error}
              <button 
                onClick={() => setError('')}
                className="float-right text-red-200 hover:text-white"
              >
                ✕
              </button>
            </div>
          )}

          {/* Loading State */}
          {isLoading && (
            <div className="text-center py-12">
              <div className="loading-spinner mx-auto mb-4" style={{ width: '2rem', height: '2rem' }}></div>
              <p className="text-gray-400">Loading your rooms...</p>
            </div>
          )}

          {/* Rooms Grid */}
          {!isLoading && (
            <>
              <div className="flex justify-between items-center mb-6">
                <h2 className="text-xl font-semibold text-gray-200">
                  My Rooms ({rooms.length})
                </h2>
                <button
                  onClick={() => setShowCreateModal(true)}
                  className="btn-primary"
                >
                  ➕ Create Room
                </button>
              </div>

              <div className="room-grid">
                {/* Create Room Card */}
                <div 
                  className="create-room-card"
                  onClick={() => setShowCreateModal(true)}
                >
                  <div className="create-room-icon">➕</div>
                  <div className="create-room-text">Create New Room</div>
                  <div className="create-room-subtitle">
                    Start a new collaborative debate space
                  </div>
                </div>

                {/* Room Cards */}
                {rooms.map((room) => (
                  <RoomCard
                    key={room._id}
                    room={room}
                    onEnterRoom={handleEnterRoom}
                    onManageRoom={handleManageRoom}
                  />
                ))}
              </div>

              {/* Empty State */}
              {rooms.length === 0 && !isLoading && (
                <div className="text-center py-16">
                  <div className="text-6xl mb-4">🏠</div>
                  <h3 className="text-xl font-semibold text-gray-300 mb-2">
                    No rooms yet
                  </h3>
                  <p className="text-gray-400 mb-6">
                    Create your first debate room or join one using an invite code
                  </p>
                  <button
                    onClick={() => setShowCreateModal(true)}
                    className="btn-primary"
                  >
                    Create Your First Room
                  </button>
                </div>
              )}
            </>
          )}
        </div>
      </div>

      {/* Create Room Modal */}
      <CreateRoomModal
        isOpen={showCreateModal}
        onClose={() => setShowCreateModal(false)}
        onCreateRoom={handleCreateRoom}
      />

      {/* Manage Room Modal */}
      <ManageRoomModal
        isOpen={showManageModal}
        onClose={() => setShowManageModal(false)}
        room={selectedRoom}
        onRoomDeleted={handleDeleteRoom}
      />

      {/* Friends Modal */}
      <FriendsModal
        isOpen={showFriendsModal}
        onClose={() => setShowFriendsModal(false)}
      />

      {/* Floating Chat */}
      {token && <ChatFloat />}
    </div>
  );
}
