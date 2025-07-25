import React, { useState, useEffect } from 'react';
import { useNavigate, useParams } from 'react-router-dom';
import { socket, joinRoom, leaveRoom } from '../../lib/socket';
import { getCurrentUser } from '../../lib/auth';
import { roomAPI } from '../../lib/roomAPI';

import GraphView from '../../components/GraphView';
import RoomSidebar from '../../components/RoomSidebar';
import ToolBar from '../../components/ToolBar';
import EntityForm from '../../components/EntityForm';
import ChallengeResponseModal from '../../components/ChallengeResponseModal';

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
  const [challengeModal, setChallengeModal] = useState({
    isOpen: false,
    challenges: [],
    targetType: null,
    targetId: null
  });
  const [showSidebar, setShowSidebar] = useState(false);
  
  // Check authentication and room access on component mount
  useEffect(() => {
    const token = localStorage.getItem('token');
    if (!token) {
      console.log('No token found, redirecting to home');
      navigate('/');
      return;
    }

    // If no roomId, redirect to home (legacy route)
    if (!roomId) {
      console.log('No room ID provided, redirecting to home');
      navigate('/');
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

    const handleDebateImportNotification = (importData) => {
      console.log('📥 Debate structure imported:', importData);

      // Show notification about the import
      const message = `🚀 Debate Updated!\n\n` +
        `"${importData.summary.title}" was imported by ${importData.importedBy}\n\n` +
        `Changes:\n` +
        `• ${importData.summary.nodes} nodes ${importData.clearExisting ? 'replaced' : 'added'}\n` +
        `• ${importData.summary.edges} connections ${importData.clearExisting ? 'replaced' : 'added'}\n` +
        `• Room settings ${importData.summary.roomUpdated ? 'updated' : 'unchanged'}\n\n` +
        `The debate view has been automatically refreshed.`;

      alert(message);
    };

    const handleUndoNotification = (undoData) => {
      console.log('↶ Import undone:', undoData);

      const message = `↶ Import Undone!\n\n` +
        `${undoData.undoneBy} has restored the debate to its previous state.\n\n` +
        `The view has been automatically refreshed.`;

      alert(message);
    };

    socket.on('challenge-response-notification', handleChallengeNotification);
    socket.on('debate-structure-imported', handleDebateImportNotification);
    socket.on('import-undone', handleUndoNotification);

    return () => {
      socket.off('challenge-response-notification', handleChallengeNotification);
      socket.off('debate-structure-imported', handleDebateImportNotification);
      socket.off('import-undone', handleUndoNotification);
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
              onClick={() => setShowSidebar(!showSidebar)}
              className="nav-button btn-secondary"
              title="Toggle room sidebar"
            >
              📋 {showSidebar ? 'Hide' : 'Show'} Info
            </button>
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

      {/* Main Content Area */}
      <div className="app-main-content" style={{ display: 'flex', height: 'calc(100vh - 60px)' }}>
        <GraphView
          graphData={data}
          dimensions={dimensions}
          selection={selection}
          setSelection={setSelection}
          roomId={roomId}
          draggedNodeId={draggedNodeId}
          setDraggedNodeId={setDraggedNodeId}
          onChallengeClick={handleChallengeClick}
          currentUserId={currentUser}
          style={{ flex: showSidebar ? '1 1 70%' : '1 1 100%' }}
        />

        {/* Room Sidebar */}
        {showSidebar && room && (
          <div className="room-sidebar-container" style={{
            width: '30%',
            minWidth: '300px',
            borderLeft: '1px solid var(--color-gray-600)',
            backgroundColor: 'var(--color-gray-900)'
          }}>
            <RoomSidebar
              room={room}
              graphData={data}
              onClose={() => setShowSidebar(false)}
            />
          </div>
        )}
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
    </>
  );
}
