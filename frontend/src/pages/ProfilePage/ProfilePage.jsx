import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { getCurrentUser } from '../../lib/auth';

export default function ProfilePage() {
  const navigate = useNavigate();
  const [user, setUser] = useState(null);
  const [isEditing, setIsEditing] = useState(false);
  const [username, setUsername] = useState('');
  const [isLoading, setIsLoading] = useState(true);
  const [isSaving, setIsSaving] = useState(false);
  const [error, setError] = useState('');

  useEffect(() => {
    const token = localStorage.getItem('token');
    if (!token) {
      navigate('/');
      return;
    }

    const currentUser = getCurrentUser();
    // console.log('📱 Current user from token:', currentUser); // Commented to reduce log spam
    
    if (currentUser) {
      setUser(currentUser);
      setUsername(currentUser.username || currentUser.email.split('@')[0]);
      setIsLoading(false);
    } else {
      console.error('❌ Failed to get current user from token');
      navigate('/');
    }
  }, [navigate]);

  const handleSaveProfile = async () => {
    if (!username.trim()) {
      setError('Username cannot be empty');
      return;
    }

    setIsSaving(true);
    setError('');

    try {
      const token = localStorage.getItem('token');
      console.log('🔐 Making profile update request with token:', token ? 'present' : 'missing');
      console.log('📝 Username to update:', username.trim());
      
      const response = await fetch(`${import.meta.env.VITE_API_URL || 'http://localhost:3000'}/api/auth/profile`, {
        method: 'PUT',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${token}`
        },
        body: JSON.stringify({ username: username.trim() })
      });

      console.log('📡 Response status:', response.status);
      
      if (response.ok) {
        const updatedUser = await response.json();
        console.log('✅ Profile updated successfully:', updatedUser);
        
        setUser({
          userId: updatedUser.userId,
          email: updatedUser.email,
          username: updatedUser.username,
          createdAt: updatedUser.createdAt
        });
        setIsEditing(false);
        
        // Update the token with new user data
        localStorage.setItem('token', updatedUser.token);
      } else {
        const errorData = await response.json();
        console.error('❌ Profile update failed:', errorData);
        setError(errorData.message || 'Failed to update profile');
      }
    } catch (error) {
      console.error('Profile update error:', error);
      setError('Network error occurred');
    } finally {
      setIsSaving(false);
    }
  };

  const handleLogout = () => {
    localStorage.removeItem('token');
    navigate('/');
  };

  const formatDate = (dateString) => {
    return new Date(dateString).toLocaleDateString('en-US', {
      year: 'numeric',
      month: 'long',
      day: 'numeric',
      hour: '2-digit',
      minute: '2-digit'
    });
  };

  if (isLoading) {
    return (
      <div style={{
        minHeight: '100vh',
        background: 'linear-gradient(135deg, #0f172a 0%, #1e293b 100%)',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        color: 'white'
      }}>
        <div style={{ textAlign: 'center' }}>
          <div style={{
            width: '48px',
            height: '48px',
            border: '4px solid rgba(255, 255, 255, 0.1)',
            borderTop: '4px solid #60a5fa',
            borderRadius: '50%',
            animation: 'spin 1s linear infinite',
            margin: '0 auto 16px'
          }}></div>
          <p>Loading profile...</p>
        </div>
      </div>
    );
  }

  return (
    <div style={{
      minHeight: '100vh',
      background: 'linear-gradient(135deg, #0f172a 0%, #1e293b 100%)',
      color: 'white'
    }}>
      {/* Navigation Header */}
      <div style={{
        background: 'rgba(255, 255, 255, 0.05)',
        backdropFilter: 'blur(10px)',
        borderBottom: '1px solid rgba(255, 255, 255, 0.1)',
        padding: '16px 32px'
      }}>
        <div style={{
          display: 'flex',
          justifyContent: 'space-between',
          alignItems: 'center',
          maxWidth: '1200px',
          margin: '0 auto'
        }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: '12px' }}>
            <button
              onClick={() => navigate('/')}
              style={{
                background: 'none',
                border: 'none',
                color: '#60a5fa',
                fontSize: '24px',
                cursor: 'pointer',
                padding: '8px',
                borderRadius: '8px',
                transition: 'background 0.2s ease'
              }}
              onMouseOver={(e) => e.target.style.background = 'rgba(96, 165, 250, 0.1)'}
              onMouseOut={(e) => e.target.style.background = 'none'}
            >
              ← 
            </button>
            <h1 style={{
              margin: 0,
              fontSize: '24px',
              fontWeight: '600',
              background: 'linear-gradient(135deg, #60a5fa, #a78bfa)',
              backgroundClip: 'text',
              WebkitBackgroundClip: 'text',
              color: 'transparent'
            }}>
              🗣️ DebateGraph - Profile
            </h1>
          </div>
          <button
            onClick={handleLogout}
            style={{
              padding: '8px 16px',
              background: 'linear-gradient(135deg, #dc2626, #ef4444)',
              border: 'none',
              borderRadius: '8px',
              color: 'white',
              fontSize: '14px',
              fontWeight: '600',
              cursor: 'pointer',
              transition: 'all 0.2s ease'
            }}
            onMouseOver={(e) => e.target.style.transform = 'translateY(-1px)'}
            onMouseOut={(e) => e.target.style.transform = 'translateY(0)'}
          >
            Logout
          </button>
        </div>
      </div>

      {/* Profile Content */}
      <div style={{
        maxWidth: '800px',
        margin: '0 auto',
        padding: '48px 32px'
      }}>
        <div style={{
          background: 'linear-gradient(135deg, #1e293b 0%, #334155 100%)',
          borderRadius: '16px',
          border: '1px solid rgba(255, 255, 255, 0.1)',
          boxShadow: '0 20px 40px rgba(0, 0, 0, 0.3)',
          overflow: 'hidden'
        }}>
          {/* Profile Header */}
          <div style={{
            background: 'linear-gradient(135deg, #60a5fa, #a78bfa)',
            padding: '32px',
            textAlign: 'center'
          }}>
            <div style={{
              width: '80px',
              height: '80px',
              background: 'rgba(255, 255, 255, 0.2)',
              borderRadius: '50%',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              margin: '0 auto 16px',
              fontSize: '32px'
            }}>
              👤
            </div>
            <h2 style={{
              margin: '0 0 8px 0',
              fontSize: '28px',
              fontWeight: '700',
              color: 'white'
            }}>
              {user?.username || user?.email?.split('@')[0] || 'User'}
            </h2>
            <p style={{
              margin: 0,
              fontSize: '16px',
              color: 'rgba(255, 255, 255, 0.8)'
            }}>
              Debate Participant
            </p>
          </div>

          {/* Profile Details */}
          <div style={{ padding: '32px' }}>
            {error && (
              <div style={{
                background: 'rgba(239, 68, 68, 0.1)',
                border: '1px solid rgba(239, 68, 68, 0.3)',
                borderRadius: '8px',
                padding: '12px 16px',
                marginBottom: '24px',
                color: '#fca5a5',
                fontSize: '14px'
              }}>
                {error}
              </div>
            )}

            {/* Username Section */}
            <div style={{ marginBottom: '32px' }}>
              <div style={{
                display: 'flex',
                justifyContent: 'space-between',
                alignItems: 'center',
                marginBottom: '16px'
              }}>
                <h3 style={{
                  margin: 0,
                  fontSize: '18px',
                  fontWeight: '600',
                  color: '#e2e8f0'
                }}>
                  Username
                </h3>
                {!isEditing && (
                  <button
                    onClick={() => setIsEditing(true)}
                    style={{
                      padding: '6px 12px',
                      background: 'linear-gradient(135deg, #059669, #10b981)',
                      border: 'none',
                      borderRadius: '6px',
                      color: 'white',
                      fontSize: '12px',
                      fontWeight: '600',
                      cursor: 'pointer',
                      transition: 'all 0.2s ease'
                    }}
                    onMouseOver={(e) => e.target.style.transform = 'translateY(-1px)'}
                    onMouseOut={(e) => e.target.style.transform = 'translateY(0)'}
                  >
                    ✏️ Edit
                  </button>
                )}
              </div>

              {isEditing ? (
                <div style={{ display: 'flex', gap: '12px', alignItems: 'center' }}>
                  <input
                    type="text"
                    value={username}
                    onChange={(e) => setUsername(e.target.value)}
                    style={{
                      flex: 1,
                      padding: '12px 16px',
                      background: 'rgba(255, 255, 255, 0.1)',
                      border: '1px solid rgba(255, 255, 255, 0.2)',
                      borderRadius: '8px',
                      color: 'white',
                      fontSize: '16px',
                      outline: 'none',
                      transition: 'all 0.2s ease'
                    }}
                    onFocus={(e) => {
                      e.target.style.borderColor = 'rgba(96, 165, 250, 0.5)';
                      e.target.style.boxShadow = '0 0 0 3px rgba(96, 165, 250, 0.1)';
                    }}
                    onBlur={(e) => {
                      e.target.style.borderColor = 'rgba(255, 255, 255, 0.2)';
                      e.target.style.boxShadow = 'none';
                    }}
                  />
                  <button
                    onClick={handleSaveProfile}
                    disabled={isSaving}
                    style={{
                      padding: '12px 16px',
                      background: isSaving ? '#6b7280' : 'linear-gradient(135deg, #059669, #10b981)',
                      border: 'none',
                      borderRadius: '8px',
                      color: 'white',
                      fontSize: '14px',
                      fontWeight: '600',
                      cursor: isSaving ? 'not-allowed' : 'pointer',
                      transition: 'all 0.2s ease',
                      opacity: isSaving ? 0.7 : 1
                    }}
                  >
                    {isSaving ? '⏳ Saving...' : '✅ Save'}
                  </button>
                  <button
                    onClick={() => {
                      setIsEditing(false);
                      setUsername(user?.username || user?.email?.split('@')[0] || '');
                      setError('');
                    }}
                    disabled={isSaving}
                    style={{
                      padding: '12px 16px',
                      background: 'rgba(255, 255, 255, 0.1)',
                      border: '1px solid rgba(255, 255, 255, 0.2)',
                      borderRadius: '8px',
                      color: 'white',
                      fontSize: '14px',
                      fontWeight: '600',
                      cursor: isSaving ? 'not-allowed' : 'pointer',
                      transition: 'all 0.2s ease'
                    }}
                  >
                    ❌ Cancel
                  </button>
                </div>
              ) : (
                <div style={{
                  padding: '16px',
                  background: 'rgba(255, 255, 255, 0.05)',
                  borderRadius: '8px',
                  border: '1px solid rgba(255, 255, 255, 0.1)'
                }}>
                  <p style={{
                    margin: 0,
                    fontSize: '16px',
                    color: '#f1f5f9',
                    fontWeight: '500'
                  }}>
                    {user?.username || user?.email?.split('@')[0] || 'Not set'}
                  </p>
                </div>
              )}
            </div>

            {/* Email Section */}
            <div style={{ marginBottom: '32px' }}>
              <h3 style={{
                margin: '0 0 16px 0',
                fontSize: '18px',
                fontWeight: '600',
                color: '#e2e8f0'
              }}>
                Email Address
              </h3>
              <div style={{
                padding: '16px',
                background: 'rgba(255, 255, 255, 0.05)',
                borderRadius: '8px',
                border: '1px solid rgba(255, 255, 255, 0.1)'
              }}>
                <p style={{
                  margin: 0,
                  fontSize: '16px',
                  color: '#f1f5f9'
                }}>
                  {user?.email}
                </p>
                <p style={{
                  margin: '4px 0 0 0',
                  fontSize: '12px',
                  color: '#94a3b8'
                }}>
                  Email cannot be changed
                </p>
              </div>
            </div>

            {/* Account Created Section */}
            <div>
              <h3 style={{
                margin: '0 0 16px 0',
                fontSize: '18px',
                fontWeight: '600',
                color: '#e2e8f0'
              }}>
                Member Since
              </h3>
              <div style={{
                padding: '16px',
                background: 'rgba(255, 255, 255, 0.05)',
                borderRadius: '8px',
                border: '1px solid rgba(255, 255, 255, 0.1)'
              }}>
                <p style={{
                  margin: 0,
                  fontSize: '16px',
                  color: '#f1f5f9'
                }}>
                  {user?.createdAt ? formatDate(user.createdAt) : 'Unknown'}
                </p>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
