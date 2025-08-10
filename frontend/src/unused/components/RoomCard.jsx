import { useState } from 'react';

export function RoomCard({ room, onEnterRoom, onManageRoom }) {
  const [isLoading, setIsLoading] = useState(false);

  const handleEnterRoom = async () => {
    setIsLoading(true);
    try {
      await onEnterRoom(room._id);
    } finally {
      setIsLoading(false);
    }
  };

  const formatDate = (dateString) => {
    const date = new Date(dateString);
    const now = new Date();
    const diffTime = Math.abs(now - date);
    const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24));
    
    if (diffDays === 1) return 'Yesterday';
    if (diffDays < 7) return `${diffDays} days ago`;
    if (diffDays < 30) return `${Math.ceil(diffDays / 7)} weeks ago`;
    return date.toLocaleDateString();
  };

  const getRoleColor = (role) => {
    switch (role) {
      case 'owner': return 'var(--color-warning)';
      case 'admin': return 'var(--color-primary)';
      case 'member': return 'var(--color-success)';
      default: return 'var(--color-gray-400)';
    }
  };

  return (
    <div className="room-card animate-fade-in" onClick={handleEnterRoom}>
      <div className="room-card-header">
        <h3 className="room-name">{room.name}</h3>
        <span 
          className="room-role" 
          style={{ 
            background: `${getRoleColor(room.userRole)}20`,
            color: getRoleColor(room.userRole),
            borderColor: `${getRoleColor(room.userRole)}40`
          }}
        >
          {room.userRole}
        </span>
      </div>

      {room.description && (
        <p className="room-description">{room.description}</p>
      )}

      <div className="room-stats">
        <div className="room-member-count">
          <span>👥</span>
          <span>{room.memberCount} member{room.memberCount !== 1 ? 's' : ''}</span>
        </div>
        <div className="room-updated">
          Updated {formatDate(room.updatedAt)}
        </div>
      </div>

      <div className="room-actions" onClick={(e) => e.stopPropagation()}>
        <button 
          className="room-action-btn btn-primary"
          onClick={handleEnterRoom}
          disabled={isLoading}
        >
          {isLoading ? (
            <>
              <span className="loading-spinner"></span>
              Entering...
            </>
          ) : (
            'Enter Room'
          )}
        </button>
        
        {(room.userRole === 'owner' || room.userRole === 'admin') && (
          <button 
            className="room-action-btn btn-secondary"
            onClick={(e) => {
              e.stopPropagation();
              onManageRoom(room);
            }}
          >
            Manage
          </button>
        )}
      </div>
    </div>
  );
}
