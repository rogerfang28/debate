import { useState } from 'react';

export function CreateRoomModal({ isOpen, onClose, onCreateRoom }) {
  const [formData, setFormData] = useState({
    name: '',
    description: '',
    isPublic: false,
    maxMembers: 50
  });
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState('');

  const handleSubmit = async (e) => {
    e.preventDefault();
    setError('');
    
    if (!formData.name.trim()) {
      setError('Room name is required');
      return;
    }

    setIsLoading(true);
    try {
      await onCreateRoom(formData);
      handleClose();
    } catch (err) {
      setError(err.message || 'Failed to create room');
    } finally {
      setIsLoading(false);
    }
  };

  const handleClose = () => {
    setFormData({
      name: '',
      description: '',
      isPublic: false,
      maxMembers: 50
    });
    setError('');
    setIsLoading(false);
    onClose();
  };

  const handleChange = (e) => {
    const { name, value, type, checked } = e.target;
    setFormData(prev => ({
      ...prev,
      [name]: type === 'checkbox' ? checked : value
    }));
  };

  if (!isOpen) return null;

  return (
    <div className="room-modal-overlay" onClick={handleClose}>
      <div className="room-modal animate-fade-in" onClick={(e) => e.stopPropagation()}>
        <div className="room-modal-header">
          <h2 className="room-modal-title">Create New Room</h2>
          <button 
            className="room-modal-close"
            onClick={handleClose}
            aria-label="Close modal"
          >
            ✕
          </button>
        </div>

        <form onSubmit={handleSubmit}>
          <div className="room-form-group">
            <label htmlFor="room-name" className="room-form-label">
              Room Name *
            </label>
            <input
              id="room-name"
              name="name"
              type="text"
              value={formData.name}
              onChange={handleChange}
              className="room-form-input"
              placeholder="Enter room name..."
              maxLength={100}
              required
            />
          </div>

          <div className="room-form-group">
            <label htmlFor="room-description" className="room-form-label">
              Description
            </label>
            <textarea
              id="room-description"
              name="description"
              value={formData.description}
              onChange={handleChange}
              className="room-form-textarea"
              placeholder="Describe what this debate room is about..."
              maxLength={500}
              rows={3}
            />
          </div>

          <div className="room-form-group">
            <label htmlFor="room-max-members" className="room-form-label">
              Maximum Members
            </label>
            <input
              id="room-max-members"
              name="maxMembers"
              type="number"
              value={formData.maxMembers}
              onChange={handleChange}
              className="room-form-input"
              min={1}
              max={200}
            />
          </div>

          <div className="room-form-group">
            <div className="room-form-checkbox-group">
              <input
                id="room-is-public"
                name="isPublic"
                type="checkbox"
                checked={formData.isPublic}
                onChange={handleChange}
                className="room-form-checkbox"
              />
              <label htmlFor="room-is-public" className="room-form-label">
                Make room publicly discoverable
              </label>
            </div>
          </div>

          {error && (
            <div className="bg-red-900/50 border border-red-600/50 text-red-200 p-3 rounded-lg mb-4">
              {error}
            </div>
          )}

          <div className="room-form-actions">
            <button
              type="button"
              onClick={handleClose}
              className="btn-secondary"
              disabled={isLoading}
            >
              Cancel
            </button>
            <button
              type="submit"
              className="btn-primary"
              disabled={isLoading}
            >
              {isLoading ? (
                <>
                  <span className="loading-spinner"></span>
                  Creating...
                </>
              ) : (
                'Create Room'
              )}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}
