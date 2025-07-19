import { useState, useEffect } from 'react';
import { roomAPI } from '../lib/roomAPI';
import { friendsAPI } from '../lib/friendsAPI';

export function ManageRoomModal({ isOpen, onClose, room, onRoomDeleted }) {
  const [inviteCode, setInviteCode] = useState('');
  const [isGeneratingCode, setIsGeneratingCode] = useState(false);
  const [showDeleteConfirm, setShowDeleteConfirm] = useState(false);
  const [isDeleting, setIsDeleting] = useState(false);
  const [isPublic, setIsPublic] = useState(false);
  const [isUpdatingPrivacy, setIsUpdatingPrivacy] = useState(false);
  const [nodeCount, setNodeCount] = useState(0);
  const [edgeCount, setEdgeCount] = useState(0);
  const [isLoadingCounts, setIsLoadingCounts] = useState(false);
  const [showFriendInvite, setShowFriendInvite] = useState(false);
  const [friends, setFriends] = useState([]);
  const [isSendingInvite, setIsSendingInvite] = useState(null);

  // Sync isPublic state with room prop whenever room changes
  useEffect(() => {
    if (room) {
      setIsPublic(room.isPublic || false);
    }
  }, [room]);

  // Fetch node and edge counts when room changes
  useEffect(() => {
    const fetchGraphCounts = async () => {
      if (room?._id) {
        setIsLoadingCounts(true);
        try {
          const graphData = await roomAPI.getRoomGraph(room._id);
          setNodeCount(graphData.nodes?.length || 0);
          setEdgeCount(graphData.edges?.length || 0);
        } catch (error) {
          console.error('Failed to fetch graph data:', error);
          setNodeCount(0);
          setEdgeCount(0);
        } finally {
          setIsLoadingCounts(false);
        }
      }
    };

    if (room && isOpen) {
      fetchGraphCounts();
    }
  }, [room, isOpen]);

  if (!isOpen || !room) return null;

  const handleGenerateInviteCode = async () => {
    setIsGeneratingCode(true);
    try {
      const response = await fetch(`http://localhost:3000/api/rooms/${room._id}/invite-code`, {
        method: 'POST',
        headers: {
          'Authorization': `Bearer ${localStorage.getItem('token')}`,
          'Content-Type': 'application/json'
        }
      });

      if (response.ok) {
        const data = await response.json();
        setInviteCode(data.inviteCode);
      } else {
        console.error('Failed to generate invite code');
      }
    } catch (error) {
      console.error('Error generating invite code:', error);
    } finally {
      setIsGeneratingCode(false);
    }
  };

  const handlePrivacyToggle = async (newIsPublic) => {
    setIsUpdatingPrivacy(true);
    try {
      const response = await fetch(`http://localhost:3000/api/rooms/${room._id}`, {
        method: 'PUT',
        headers: {
          'Authorization': `Bearer ${localStorage.getItem('token')}`,
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({
          isPublic: newIsPublic
        })
      });

      if (response.ok) {
        setIsPublic(newIsPublic);
        // Update the room object if needed
        room.isPublic = newIsPublic;
      } else {
        console.error('Failed to update room privacy');
        // Reset the toggle to previous state on error
        setIsPublic(room.isPublic);
      }
    } catch (error) {
      console.error('Error updating room privacy:', error);
      // Reset the toggle to previous state on error
      setIsPublic(room.isPublic);
    } finally {
      setIsUpdatingPrivacy(false);
    }
  };

  const handleDeleteRoom = async () => {
    setIsDeleting(true);
    try {
      await roomAPI.deleteRoom(room._id);
      // Notify parent component that room was deleted
      if (onRoomDeleted) {
        onRoomDeleted(room._id);
      }
      onClose();
    } catch (error) {
      console.error('Failed to delete room:', error);
      alert('Failed to delete room. Please try again.');
    } finally {
      setIsDeleting(false);
      setShowDeleteConfirm(false);
    }
  };

  const copyInviteCode = () => {
    navigator.clipboard.writeText(inviteCode);
    // You could add a toast notification here
  };

  const loadFriends = async () => {
    try {
      const data = await friendsAPI.getFriends();
      setFriends(data.friends);
    } catch (error) {
      console.error('Failed to load friends:', error);
    }
  };

  const handleInviteFriend = async (friendId) => {
    setIsSendingInvite(friendId);
    try {
      await friendsAPI.sendRoomInvitation(friendId, room._id);
      alert('Room invitation sent!');
    } catch (error) {
      console.error('Failed to send room invitation:', error);
      alert('Failed to send room invitation');
    } finally {
      setIsSendingInvite(null);
    }
  };

  return (
    <div className="room-modal-overlay" onClick={onClose}>
      <div className="room-modal" onClick={(e) => e.stopPropagation()}>
        <div className="room-modal-header">
          <h2 className="room-modal-title">Manage Room: {room.name}</h2>
          <button
            onClick={onClose}
            className="room-modal-close"
          >
            ×
          </button>
        </div>

        <div className="room-form-group">
          <label className="room-form-label">Room Information</label>
          <div style={{ marginBottom: '1rem' }}>
            <p><strong>Description:</strong> {room.description || 'No description'}</p>
            <p><strong>Members:</strong> {room.memberCount}</p>
            <p>
              <strong>Nodes:</strong> {isLoadingCounts ? 'Loading...' : nodeCount}
            </p>
            <p>
              <strong>Edges:</strong> {isLoadingCounts ? 'Loading...' : edgeCount}
            </p>
            <p><strong>Your Role:</strong> {room.userRole}</p>
            
            {/* Privacy Setting - Toggle for owners/admins, display only for members */}
            {(room.userRole === 'owner' || room.userRole === 'admin') ? (
              <div style={{ display: 'flex', alignItems: 'center', gap: '0.75rem', marginTop: '0.5rem' }}>
                <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
                  <label style={{
                    position: 'relative',
                    display: 'inline-block',
                    width: '50px',
                    height: '24px',
                    cursor: isUpdatingPrivacy ? 'not-allowed' : 'pointer',
                    opacity: isUpdatingPrivacy ? 0.6 : 1
                  }}>
                    <input
                      type="checkbox"
                      checked={isPublic}
                      onChange={(e) => handlePrivacyToggle(e.target.checked)}
                      disabled={isUpdatingPrivacy}
                      style={{ opacity: 0, width: 0, height: 0 }}
                    />
                    <span style={{
                      position: 'absolute',
                      top: 0,
                      left: 0,
                      right: 0,
                      bottom: 0,
                      backgroundColor: isPublic ? 'var(--color-primary)' : 'var(--color-gray-600)',
                      borderRadius: '24px',
                      transition: 'background-color 0.3s ease',
                      border: '1px solid var(--color-gray-500)'
                    }}>
                      <span style={{
                        position: 'absolute',
                        content: '',
                        height: '18px',
                        width: '18px',
                        left: isPublic ? '28px' : '3px',
                        bottom: '2px',
                        backgroundColor: 'white',
                        borderRadius: '50%',
                        transition: 'left 0.3s ease',
                        boxShadow: '0 2px 4px rgba(0,0,0,0.2)'
                      }}></span>
                    </span>
                  </label>
                  <span style={{ 
                    fontSize: '0.9rem', 
                    color: isPublic ? 'var(--color-primary)' : 'var(--color-gray-400)',
                    fontWeight: isPublic ? 'bold' : 'normal'
                  }}>
                    Public
                  </span>
                </div>
                {isUpdatingPrivacy && (
                  <span style={{ fontSize: '0.8rem', color: 'var(--color-gray-400)' }}>
                    Updating...
                  </span>
                )}
              </div>
            ) : (
              <p><strong>Privacy:</strong> {room.isPublic ? 'Public' : 'Private'}</p>
            )}
          </div>
        </div>

        {(room.userRole === 'owner' || room.userRole === 'admin') && (
          <div className="room-form-group">
            <label className="room-form-label">Invite Others</label>
            <div style={{ display: 'flex', gap: '0.5rem', alignItems: 'center', marginBottom: '1rem' }}>
              <button
                onClick={handleGenerateInviteCode}
                disabled={isGeneratingCode}
                className="btn-primary"
                style={{ flex: 'none' }}
              >
                {isGeneratingCode ? 'Generating...' : 'Generate Invite Code'}
              </button>
              <button
                onClick={() => {
                  setShowFriendInvite(!showFriendInvite);
                  if (!showFriendInvite && friends.length === 0) {
                    loadFriends();
                  }
                }}
                className="btn-secondary"
                style={{ flex: 'none' }}
              >
                👥 Invite Friends
              </button>
            </div>
            
            {/* Friend Invite Section */}
            {showFriendInvite && (
              <div style={{ 
                marginBottom: '1rem', 
                padding: '1rem', 
                backgroundColor: 'var(--color-gray-700)', 
                borderRadius: '0.5rem',
                border: '1px solid var(--color-gray-600)'
              }}>
                <h4 style={{ margin: '0 0 1rem 0', color: 'var(--color-white)' }}>Invite Friends to Room</h4>
                {friends.length === 0 ? (
                  <p style={{ color: 'var(--color-gray-400)', fontStyle: 'italic' }}>
                    No friends found. Add some friends first to invite them to your rooms!
                  </p>
                ) : (
                  <div style={{ display: 'flex', flexDirection: 'column', gap: '0.5rem', maxHeight: '200px', overflowY: 'auto' }}>
                    {friends.map(friend => (
                      <div key={friend._id} style={{
                        display: 'flex',
                        justifyContent: 'space-between',
                        alignItems: 'center',
                        padding: '0.75rem',
                        backgroundColor: 'var(--color-gray-800)',
                        borderRadius: '0.375rem',
                        border: '1px solid var(--color-gray-600)'
                      }}>
                        <div>
                          <strong style={{ color: 'var(--color-white)' }}>{friend.username}</strong>
                          <p style={{ margin: '0', fontSize: '0.875rem', color: 'var(--color-gray-400)' }}>
                            {friend.email}
                          </p>
                        </div>
                        <button
                          onClick={() => handleInviteFriend(friend._id)}
                          disabled={isSendingInvite === friend._id}
                          className="btn-primary"
                          style={{ 
                            fontSize: '0.875rem', 
                            padding: '0.5rem 1rem',
                            opacity: isSendingInvite === friend._id ? 0.6 : 1
                          }}
                        >
                          {isSendingInvite === friend._id ? 'Sending...' : '📨 Invite'}
                        </button>
                      </div>
                    ))}
                  </div>
                )}
              </div>
            )}
            
            {inviteCode && (
              <div style={{ marginTop: '1rem' }}>
                <div style={{ display: 'flex', gap: '0.5rem', alignItems: 'center' }}>
                  <input
                    type="text"
                    value={inviteCode}
                    readOnly
                    className="room-form-input"
                    style={{ fontFamily: 'monospace', fontSize: '0.9rem' }}
                  />
                  <button
                    onClick={copyInviteCode}
                    className="btn-secondary"
                    style={{ flex: 'none', minWidth: '80px' }}
                  >
                    Copy
                  </button>
                </div>
                <p style={{ fontSize: '0.8rem', color: 'var(--color-gray-400)', marginTop: '0.5rem' }}>
                  Share this code with others to invite them to your room
                </p>
              </div>
            )}
          </div>
        )}

        {/* Danger Zone - Only for room owners */}
        {room.userRole === 'owner' && (
          <div className="room-form-group" style={{ borderTop: '1px solid var(--color-gray-600)', paddingTop: '1rem', marginTop: '1.5rem' }}>
            <label className="room-form-label" style={{ color: 'var(--color-red-400)' }}>Danger Zone</label>
            <div style={{ marginBottom: '1rem' }}>
              <p style={{ fontSize: '0.9rem', color: 'var(--color-gray-400)', marginBottom: '1rem' }}>
                Deleting this room will permanently remove all debates, nodes, edges, and member access. This action cannot be undone.
              </p>
              <button
                onClick={() => setShowDeleteConfirm(true)}
                className="btn-danger"
              >
                🗑️ Delete Room
              </button>
            </div>
          </div>
        )}

        {/* Delete Confirmation Dialog */}
        {showDeleteConfirm && (
          <div style={{
            position: 'fixed',
            top: 0,
            left: 0,
            right: 0,
            bottom: 0,
            backgroundColor: 'rgba(0, 0, 0, 0.8)',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            zIndex: 10000
          }}>
            <div style={{
              backgroundColor: 'var(--color-gray-800)',
              padding: '2rem',
              borderRadius: '0.5rem',
              maxWidth: '400px',
              width: '90%',
              border: '1px solid var(--color-gray-600)'
            }}>
              <h3 style={{ color: 'var(--color-red-400)', marginBottom: '1rem' }}>⚠️ Confirm Deletion</h3>
              <p style={{ marginBottom: '1.5rem', color: 'var(--color-gray-300)' }}>
                Are you sure you want to delete "<strong>{room.name}</strong>"? 
                This will permanently remove:
              </p>
              <ul style={{ marginBottom: '1.5rem', color: 'var(--color-gray-400)', paddingLeft: '1.5rem' }}>
                <li>All debate nodes and connections</li>
                <li>All member access</li>
                <li>Room settings and history</li>
              </ul>
              <p style={{ marginBottom: '1.5rem', color: 'var(--color-red-400)', fontWeight: 'bold' }}>
                This action cannot be undone!
              </p>
              <div style={{ display: 'flex', gap: '0.5rem', justifyContent: 'flex-end' }}>
                <button
                  onClick={() => setShowDeleteConfirm(false)}
                  disabled={isDeleting}
                  className="btn-secondary"
                >
                  Cancel
                </button>
                <button
                  onClick={handleDeleteRoom}
                  disabled={isDeleting}
                  className="btn-danger"
                  style={{
                    cursor: isDeleting ? 'not-allowed' : 'pointer',
                    opacity: isDeleting ? 0.6 : 1
                  }}
                >
                  {isDeleting ? 'Deleting...' : 'Delete Forever'}
                </button>
              </div>
            </div>
          </div>
        )}

        <div className="room-form-actions">
          <button
            onClick={onClose}
            className="btn-secondary"
          >
            Close
          </button>
        </div>
      </div>
    </div>
  );
}
