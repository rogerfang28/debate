import { useState } from 'react';

export function JoinRoomForm({ onJoinRoom }) {
  const [inviteCode, setInviteCode] = useState('');
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState('');

  const handleSubmit = async (e) => {
    e.preventDefault();
    setError('');
    
    const code = inviteCode.trim().toUpperCase();
    if (!code) {
      setError('Please enter an invite code');
      return;
    }

    if (code.length !== 8) {
      setError('Invite code must be 8 characters');
      return;
    }

    setIsLoading(true);
    try {
      await onJoinRoom(code);
      setInviteCode('');
    } catch (err) {
      setError(err.message || 'Failed to join room');
    } finally {
      setIsLoading(false);
    }
  };

  const handleInputChange = (e) => {
    const value = e.target.value.toUpperCase().replace(/[^A-Z0-9]/g, '');
    if (value.length <= 8) {
      setInviteCode(value);
      setError('');
    }
  };

  return (
    <div className="join-room-section animate-fade-in">
      <h2 className="join-room-title">Join a Room</h2>
      <p className="text-gray-400 mb-4">
        Have an invite code? Enter it below to join an existing debate room.
      </p>
      
      <form onSubmit={handleSubmit} className="join-room-form">
        <div className="join-room-input-group">
          <label htmlFor="invite-code" className="room-form-label">
            Invite Code
          </label>
          <input
            id="invite-code"
            type="text"
            value={inviteCode}
            onChange={handleInputChange}
            className="join-room-input"
            placeholder="ABC12345"
            maxLength={8}
            style={{ fontFamily: 'monospace' }}
          />
          {error && (
            <div className="text-red-200 text-sm mt-2">
              {error}
            </div>
          )}
        </div>
        
        <button
          type="submit"
          className="join-room-btn btn-success"
          disabled={isLoading || !inviteCode.trim()}
        >
          {isLoading ? (
            <>
              <span className="loading-spinner"></span>
              Joining...
            </>
          ) : (
            'Join Room'
          )}
        </button>
      </form>
    </div>
  );
}
