import React, { useState, useEffect } from 'react';
import { useNavigate, useParams } from 'react-router-dom';
import { socket, joinRoom, leaveRoom } from '../../lib/socket';
import { getCurrentUser } from '../../lib/auth';
import { roomAPI } from '../../lib/roomAPI';

import GraphView from '../../components/Graph/GraphView.jsx';
import ToolBar from '../../components/ToolBar';
import EntityForm from '../../components/EntityForm';
import ChallengeResponseModal from '../../components/ChallengeResponseModal';
import RoomSidebar from '../../components/RoomSidebar';

import {
  handleAddNode,
  handleAddConnectedNode,
  handleChallenge,
  handleDelete,
  handleSubmitForm
} from './handleActions.js';

import { useInitializeGraph } from './initializeGraph.js';
import { useWindowResize } from './windowResize.js';
import { useGraphState } from './useGraphState.js';

export default function GraphPage() {
  const navigate = useNavigate();
  const { roomId } = useParams();
  const [room, setRoom] = useState(null);
  const [isLoading, setIsLoading] = useState(true);
  const [error, setError] = useState('');
  const [draggedNodeId, setDraggedNodeId] = useState(null);
  const [isMembersPanelOpen, setIsMembersPanelOpen] = useState(false);
  const [distance, setDistance] = useState(250); 
  const [challengeModal, setChallengeModal] = useState({
    isOpen: false,
    challenges: [],
    targetType: null,
    targetId: null
  });
  
  // Check authentication and room access on component mount
  useEffect(() => {
    const token = localStorage.getItem('token');
    if (!token) {
      console.log('No token found, redirecting to home');
      navigate('/home');
      return;
    }

    // If no roomId, redirect to home (legacy route)
    if (!roomId) {
      console.log('No room ID provided, redirecting to home');
      navigate('/home');
      return;
    }

    // Load room data and join
    loadRoomAndJoin();

    // Cleanup when leaving
    return () => {
      leaveRoom();
    };
  }, [navigate, roomId]);
  
  // Handle challenge response notifications
  useEffect(() => {
    const handleChallengeNotification = (notification) => {
      console.log('📢 Challenge response received:', notification);
      
      // Show notification to user
      const message = notification.action === 'accepted' 
        ? `🗑️ Challenge ACCEPTED: ${notification.targetType} was removed by ${notification.responder}\nReason: ${notification.reason}`
        : `✋ Challenge DENIED by ${notification.responder}\nReason: ${notification.reason}`;
      
      // Show alert for now (could be replaced with a better notification system)
      alert(message);
    };

    socket.on('challenge-response-notification', handleChallengeNotification);

    return () => {
      socket.off('challenge-response-notification', handleChallengeNotification);
    };
  }, []);
  
  const {
    dimensions,
    setDimensions,
    data,
    setData,
    selection,
    setSelection,
    form,
    setForm
  } = useGraphState();

  console.log("Graph data on render:", data);
  const currentUser = getCurrentUser()?.userId; // Get actual user ID from JWT token

  useInitializeGraph(setData, roomId, draggedNodeId); // Pass roomId and draggedNodeId to initialize room-specific graph
  useWindowResize(setDimensions);

  const loadRoomAndJoin = async () => {
    setIsLoading(true);
    setError('');
    
    try {
      // Get room details
      const response = await roomAPI.getRoom(roomId);
      setRoom(response.room);
      
      // Simple approach: If socket not connected, force reconnect and try again
      console.log('🔍 Checking socket connection before joining room...');
      console.log('🔌 Socket connected:', socket.connected);
      
      if (!socket.connected) {
        console.log('⚠️ Socket not connected, forcing reconnection...');
        
        // Disconnect and reconnect
        socket.disconnect();
        socket.connect();
        
        // Wait a short time for connection
        await new Promise(resolve => setTimeout(resolve, 2000));
        
        // Check again
        if (!socket.connected) {
          console.log('❌ Socket still not connected after reconnect attempt');
          // Try to join anyway - sometimes it works
          joinRoom(roomId);
        } else {
          console.log('✅ Socket reconnected successfully');
          joinRoom(roomId);
        }
      } else {
        console.log('✅ Socket already connected');
        joinRoom(roomId);
      }
      
      console.log(`Joined room: ${response.room.name}`);
    } catch (err) {
      console.error('Failed to load room:', err);
      setError(err.message || 'Failed to load room');
      // Redirect to home if room access fails
      setTimeout(() => navigate('/'), 3000);
    } finally {
      setIsLoading(false);
    }
  };

  const handleChallengeClick = (challenges, targetType, targetId) => {
    setChallengeModal({
      isOpen: true,
      challenges,
      targetType,
      targetId
    });
  };

  const closeChallengeModal = () => {
    setChallengeModal({
      isOpen: false,
      challenges: [],
      targetType: null,
      targetId: null
    });
  };

  const clearSelection = () => setSelection({ type: null, item: null });

  const handleLogout = () => {
    localStorage.removeItem('token');
    leaveRoom();
    navigate('/');
  };

  // Loading state
  if (isLoading) {
    return (
      <div className="app-container">
        <div className="app-navigation">
          <div className="nav-content">
            <div className="nav-brand">
              <span className="brand-icon">🗣️</span>
              <span className="brand-text">DebateGraph</span>
            </div>
          </div>
        </div>
        
        <div className="app-main flex items-center justify-center">
          <div className="text-center">
            <div className="loading-spinner mx-auto mb-4" style={{ width: '3rem', height: '3rem' }}></div>
            <h2 className="text-xl font-semibold mb-2">Loading Room...</h2>
            <p className="text-gray-400">Connecting to the debate space</p>
          </div>
        </div>
      </div>
    );
  }

  // Error state
  if (error) {
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
                onClick={() => navigate('/')}
                className="nav-button btn-secondary"
              >
                🏠 Home
              </button>
            </div>
          </div>
        </div>
        
        <div className="app-main flex items-center justify-center">
          <div className="text-center max-w-md">
            <div className="text-6xl mb-4">⚠️</div>
            <h2 className="text-xl font-semibold mb-2 text-red-400">Access Denied</h2>
            <p className="text-gray-300 mb-6">{error}</p>
            <p className="text-gray-400 text-sm mb-4">Redirecting to home page...</p>
            <button 
              onClick={() => navigate('/')}
              className="btn-primary"
            >
              Return Home
            </button>
          </div>
        </div>
      </div>
    );
  }

  return (
    <>
      {/* Navigation Header with Room Info */}
      <div className="app-navigation">
        <div className="nav-content">
          <div className="nav-brand" onClick={() => navigate('/')}>
            <span className="brand-icon">🗣️</span>
            <span className="brand-text">DebateGraph</span>
          </div>
          
          {room && (
            <div className="flex items-center gap-4">
              <div className="text-center">
                <div className="text-sm font-semibold text-gray-200">{room.name}</div>
                <div className="text-xs text-gray-400">
                  {room.memberCount} member{room.memberCount !== 1 ? 's' : ''} • {room.userRole}
                </div>
              </div>
            </div>
          )}
          
          <div className="nav-actions">
            <button 
              onClick={() => navigate('/')}
              className="nav-button btn-secondary"
            >
              🏠 Home
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

      <GraphView
        graphData={data}
        dimensions={dimensions}
        linkDistance={distance}
        selection={selection}
        setSelection={setSelection}
        roomId={roomId}
        draggedNodeId={draggedNodeId}
        setDraggedNodeId={setDraggedNodeId}
        onChallengeClick={handleChallengeClick}
        currentUserId={currentUser}
      />
      {/* edge‑length slider, doesn't reload graph when used, have to click on a node to reload */}
      <div style={{ position: 'fixed', bottom: 16, right: 16, zIndex: 20 }}>
        <label className="text-xs text-white block mb-1">
          Edge length: {distance}px
        </label>
        <input
          type="range"
          min="50"
          max="400"
          value={distance}
          onChange={e => setDistance(+e.target.value)}
        />
      </div>

      <ToolBar
        selection={selection}
        onAddNode={() => handleAddNode(setForm)}
        onAddConnectedNode={node => handleAddConnectedNode(node, setForm)}
        onChallenge={() => handleChallenge(selection, setForm)}
        onDelete={() => handleDelete(selection, setSelection, roomId)}
        onClearSelection={clearSelection}
      />
      
      {form && (
        <div>
          {console.log('🎭 About to render EntityForm with form:', form)}
          <EntityForm
            form={form}
            roomId={roomId}
            onSubmit={formData =>
              handleSubmitForm(form, formData, setForm, setSelection, currentUser, roomId)
            }
            onCancel={() => setForm(null)}
          />
        </div>
      )}

      {challengeModal.isOpen && (
        <ChallengeResponseModal
          challenges={challengeModal.challenges}
          targetType={challengeModal.targetType}
          targetId={challengeModal.targetId}
          onClose={closeChallengeModal}
          roomId={roomId}
        />
      )}

      {/* Room Sidebar */}
      <RoomSidebar
        room={room}
        isOpen={isMembersPanelOpen}
        onToggle={() => setIsMembersPanelOpen(!isMembersPanelOpen)}
      />
    </>
  );
}
