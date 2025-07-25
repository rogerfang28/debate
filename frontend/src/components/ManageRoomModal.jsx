import React from 'react';
import RoomPrivacySettings from './RoomPrivacySettings';
import RoomSettings from './RoomSettings';
import RoomMemberManagement from './RoomMemberManagement';
import RoomJsonManagement from './RoomJsonManagement';
import RoomDangerZone from './RoomDangerZone';

/**
 * Refactored ManageRoomModal Component
 * Now uses smaller, focused components for better maintainability
 */
export function ManageRoomModal({ isOpen, onClose, room, onRoomDeleted }) {
  const handleRoomUpdate = (updatedRoom) => {
    // This could trigger a parent component refresh if needed
    console.log('Room updated:', updatedRoom);
  };

  const handleImportSuccess = (result) => {
    console.log('Import successful:', result);
    // Could trigger room data refresh here
  };

  if (!isOpen || !room) return null;

  return (
    <div className="room-modal-overlay" onClick={onClose}>
      <div className="room-modal" onClick={(e) => e.stopPropagation()}>
        <div className="room-modal-header">
          <h2 className="room-modal-title">🛠️ Manage Room: {room.name}</h2>
          <button
            onClick={onClose}
            className="room-modal-close"
          >
            ×
          </button>
        </div>

        <div className="room-modal-content">
          {/* Room Privacy and Basic Info */}
          <RoomPrivacySettings
            room={room}
            onRoomUpdate={handleRoomUpdate}
          />

          {/* Room Settings */}
          <RoomSettings
            room={room}
            onSettingsUpdate={handleRoomUpdate}
          />

          {/* Member Management */}
          <RoomMemberManagement
            room={room}
            onRoomUpdate={handleRoomUpdate}
          />

          {/* JSON Management */}
          <RoomJsonManagement
            room={room}
            onImportSuccess={handleImportSuccess}
          />

          {/* Danger Zone */}
          <RoomDangerZone
            room={room}
            onRoomDeleted={onRoomDeleted}
            onClose={onClose}
          />
        </div>

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
