import { useState } from 'react';
import { socket } from '../lib/socket';

export default function ChallengeResponseModal({ challenges, targetType, targetId, onClose, roomId }) {
  const [selectedChallenge, setSelectedChallenge] = useState(null);
  const [responseReason, setResponseReason] = useState('');
  const [isSubmitting, setIsSubmitting] = useState(false);

  if (!challenges || challenges.length === 0) {
    return null;
  }

  const pendingChallenges = challenges.filter(c => c.status === 'pending');

  if (pendingChallenges.length === 0) {
    return null;
  }

  const handleResponse = async (challengeId, action) => {
    if (!challengeId || !action) return;
    
    // Make reasoning mandatory
    if (!responseReason.trim()) {
      alert('Please provide a reason for your response.');
      return;
    }
    
    setIsSubmitting(true);
    
    try {
      socket.emit('respond-to-challenge', {
        challengeId,
        action, // 'accepted' or 'denied'
        reason: responseReason.trim(),
        room: roomId
      });
      
      // Close modal after response
      onClose();
    } catch (error) {
      console.error('Error responding to challenge:', error);
    } finally {
      setIsSubmitting(false);
    }
  };

  return (
    <div 
      style={{
        position: 'fixed',
        top: '0',
        left: '0',
        width: '100vw',
        height: '100vh',
        background: 'rgba(0, 0, 0, 0.7)',
        backdropFilter: 'blur(4px)',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        zIndex: 200
      }}
      onClick={onClose}
    >
      <div 
        style={{
          background: 'linear-gradient(135deg, #1e293b 0%, #334155 100%)',
          color: '#fff',
          padding: '32px',
          borderRadius: '12px',
          border: '1px solid rgba(255, 255, 255, 0.1)',
          boxShadow: '0 20px 40px rgba(0, 0, 0, 0.3)',
          minWidth: '500px',
          maxWidth: '600px',
          maxHeight: '80vh',
          overflowY: 'auto'
        }}
        onClick={e => e.stopPropagation()}
      >
        <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: '24px' }}>
          <h3 style={{
            margin: 0,
            fontSize: '20px',
            fontWeight: '600',
            background: 'linear-gradient(135deg, #ef4444, #f87171)',
            backgroundClip: 'text',
            WebkitBackgroundClip: 'text',
            color: 'transparent'
          }}>
            🚨 Respond to Challenge
          </h3>
          <button 
            onClick={onClose}
            style={{
              background: 'none',
              border: 'none',
              color: '#9ca3af',
              fontSize: '18px',
              cursor: 'pointer',
              padding: '4px 8px',
              borderRadius: '4px',
              transition: 'color 0.2s ease'
            }}
            onMouseOver={(e) => e.target.style.color = '#fff'}
            onMouseOut={(e) => e.target.style.color = '#9ca3af'}
          >
            ✕
          </button>
        </div>

        <div style={{ marginBottom: '24px' }}>
          <p style={{ 
            fontSize: '14px', 
            color: '#d1d5db', 
            marginBottom: '16px',
            margin: '0 0 16px 0'
          }}>
            This {targetType} has {pendingChallenges.length} pending challenge{pendingChallenges.length > 1 ? 's' : ''}:
          </p>
          
          <div style={{ display: 'flex', flexDirection: 'column', gap: '12px' }}>
            {pendingChallenges.map((challenge, index) => (
              <div 
                key={challenge._id} 
                style={{
                  padding: '16px',
                  borderRadius: '8px',
                  border: selectedChallenge?._id === challenge._id 
                    ? '2px solid #ef4444' 
                    : '1px solid rgba(255, 255, 255, 0.2)',
                  background: selectedChallenge?._id === challenge._id 
                    ? 'rgba(239, 68, 68, 0.1)' 
                    : 'rgba(255, 255, 255, 0.05)',
                  cursor: 'pointer',
                  transition: 'all 0.2s ease'
                }}
                onClick={() => setSelectedChallenge(challenge)}
                onMouseOver={(e) => {
                  if (selectedChallenge?._id !== challenge._id) {
                    e.target.style.borderColor = 'rgba(239, 68, 68, 0.5)';
                    e.target.style.background = 'rgba(255, 255, 255, 0.1)';
                  }
                }}
                onMouseOut={(e) => {
                  if (selectedChallenge?._id !== challenge._id) {
                    e.target.style.borderColor = 'rgba(255, 255, 255, 0.2)';
                    e.target.style.background = 'rgba(255, 255, 255, 0.05)';
                  }
                }}
              >
                <div style={{ display: 'flex', alignItems: 'flex-start', justifyContent: 'space-between' }}>
                  <div style={{ flex: 1 }}>
                    <p style={{ 
                      fontSize: '14px', 
                      color: '#fff', 
                      fontWeight: '500',
                      margin: '0 0 8px 0'
                    }}>
                      Challenge #{index + 1}
                    </p>
                    <p style={{ 
                      fontSize: '12px', 
                      color: '#9ca3af',
                      margin: '0 0 8px 0'
                    }}>
                      From: {challenge.challenger?.email || 'Unknown'}
                    </p>
                    {challenge.challengeReason && (
                      <p style={{ 
                        fontSize: '14px', 
                        color: '#d1d5db',
                        fontStyle: 'italic',
                        margin: 0
                      }}>
                        "{challenge.challengeReason}"
                      </p>
                    )}
                  </div>
                  <div style={{ marginLeft: '12px' }}>
                    <span style={{
                      display: 'inline-block',
                      width: '8px',
                      height: '8px',
                      background: '#ef4444',
                      borderRadius: '50%'
                    }}></span>
                  </div>
                </div>
              </div>
            ))}
          </div>
        </div>

        {selectedChallenge && (
          <div style={{ display: 'flex', flexDirection: 'column', gap: '16px' }}>
            <div>
              <label style={{ 
                display: 'block', 
                fontSize: '14px', 
                fontWeight: '500', 
                color: '#d1d5db', 
                marginBottom: '8px' 
              }}>
                Response Reason (required) *
              </label>
              <textarea
                value={responseReason}
                onChange={(e) => setResponseReason(e.target.value)}
                placeholder="Explain your decision (required)..."
                rows={3}
                style={{
                  width: '100%',
                  padding: '12px 16px',
                  background: 'rgba(255, 255, 255, 0.1)',
                  border: '1px solid rgba(255, 255, 255, 0.2)',
                  borderRadius: '8px',
                  color: 'white',
                  fontSize: '14px',
                  outline: 'none',
                  resize: 'vertical',
                  transition: 'all 0.2s ease',
                  fontFamily: 'inherit'
                }}
                disabled={isSubmitting}
                onFocus={(e) => {
                  e.target.style.borderColor = 'rgba(239, 68, 68, 0.5)';
                  e.target.style.boxShadow = '0 0 0 3px rgba(239, 68, 68, 0.1)';
                }}
                onBlur={(e) => {
                  e.target.style.borderColor = 'rgba(255, 255, 255, 0.2)';
                  e.target.style.boxShadow = 'none';
                }}
              />
            </div>

            <div style={{ display: 'flex', gap: '12px' }}>
              <button
                onClick={() => handleResponse(selectedChallenge._id, 'accepted')}
                disabled={isSubmitting}
                style={{
                  flex: 1,
                  padding: '12px 24px',
                  background: isSubmitting ? '#6b7280' : 'linear-gradient(135deg, #059669, #10b981)',
                  border: 'none',
                  borderRadius: '8px',
                  color: 'white',
                  fontSize: '14px',
                  fontWeight: '600',
                  cursor: isSubmitting ? 'not-allowed' : 'pointer',
                  transition: 'all 0.2s ease',
                  boxShadow: '0 4px 12px rgba(16, 185, 129, 0.3)',
                  opacity: isSubmitting ? 0.6 : 1
                }}
                onMouseOver={(e) => {
                  if (!isSubmitting) {
                    e.target.style.transform = 'translateY(-1px)';
                    e.target.style.boxShadow = '0 6px 16px rgba(16, 185, 129, 0.4)';
                  }
                }}
                onMouseOut={(e) => {
                  if (!isSubmitting) {
                    e.target.style.transform = 'translateY(0)';
                    e.target.style.boxShadow = '0 4px 12px rgba(16, 185, 129, 0.3)';
                  }
                }}
              >
                {isSubmitting ? '⏳ Responding...' : '✅ Accept Challenge'}
              </button>
              <button
                onClick={() => handleResponse(selectedChallenge._id, 'denied')}
                disabled={isSubmitting}
                style={{
                  flex: 1,
                  padding: '12px 24px',
                  background: isSubmitting ? '#6b7280' : 'linear-gradient(135deg, #dc2626, #ef4444)',
                  border: 'none',
                  borderRadius: '8px',
                  color: 'white',
                  fontSize: '14px',
                  fontWeight: '600',
                  cursor: isSubmitting ? 'not-allowed' : 'pointer',
                  transition: 'all 0.2s ease',
                  boxShadow: '0 4px 12px rgba(239, 68, 68, 0.3)',
                  opacity: isSubmitting ? 0.6 : 1
                }}
                onMouseOver={(e) => {
                  if (!isSubmitting) {
                    e.target.style.transform = 'translateY(-1px)';
                    e.target.style.boxShadow = '0 6px 16px rgba(239, 68, 68, 0.4)';
                  }
                }}
                onMouseOut={(e) => {
                  if (!isSubmitting) {
                    e.target.style.transform = 'translateY(0)';
                    e.target.style.boxShadow = '0 4px 12px rgba(239, 68, 68, 0.3)';
                  }
                }}
              >
                {isSubmitting ? '⏳ Responding...' : '❌ Reject Challenge'}
              </button>
            </div>
          </div>
        )}

        {!selectedChallenge && pendingChallenges.length > 0 && (
          <p style={{ 
            fontSize: '14px', 
            color: '#9ca3af', 
            textAlign: 'center', 
            marginTop: '16px',
            margin: '16px 0 0 0',
            fontStyle: 'italic'
          }}>
            👆 Click on a challenge above to respond to it
          </p>
        )}
      </div>
    </div>
  );
}
