import { useState, useEffect, useRef } from 'react';
import { friendsAPI } from '../lib/friendsAPI';
import { socket } from '../lib/socket';
import './FriendsModal.css';

export function FriendsModal({ isOpen, onClose }) {
  const [activeTab, setActiveTab] = useState('friends');
  const [friends, setFriends] = useState([]);
  const [pendingRequests, setPendingRequests] = useState({ sent: [], received: [] });
  const [searchQuery, setSearchQuery] = useState('');
  const [searchResults, setSearchResults] = useState([]);
  const [selectedFriend, setSelectedFriend] = useState(null);
  const [chatMessages, setChatMessages] = useState([]);
  const [newMessage, setNewMessage] = useState('');
  const [isLoading, setIsLoading] = useState(false);
  const [isSearching, setIsSearching] = useState(false);
  const messagesEndRef = useRef(null);

  // Load friends and pending requests
  useEffect(() => {
    if (isOpen) {
      console.log('👥 FriendsModal: Opening, loading friends...');
      loadFriends();
      setupSocketListeners();
    } else {
      console.log('👥 FriendsModal: Closing, cleaning up...');
      // Clean up when modal closes
      socket.off('friend-request-received-modal');
      socket.off('friend-request-accepted-modal');
      socket.off('new-message-modal');
      setSelectedFriend(null);
      setChatMessages([]);
    }

    return () => {
      socket.off('friend-request-received-modal');
      socket.off('friend-request-accepted-modal');
      socket.off('new-message-modal');
    };
  }, [isOpen]); // Only depend on isOpen

  const setupSocketListeners = () => {
    // Use unique event names for modal to prevent conflicts
    // Clean up existing listeners first
    socket.off('friend-request-received-modal');
    socket.off('friend-request-accepted-modal');  
    socket.off('new-message-modal');

    console.log('👥 FriendsModal: Setting up socket listeners');

    socket.on('friend-request-received-modal', (data) => {
      console.log('👥 Friend request received in FriendsModal:', data);
      loadFriends(); // Refresh the friends list
    });

    socket.on('friend-request-accepted-modal', () => {
      console.log('✅ Friend request accepted in FriendsModal');
      loadFriends(); // Refresh the friends list
    });

    socket.on('new-message-modal', (message) => {
      console.log('📨 New message received in FriendsModal:', message);
      // If chat is open with this friend, add message to chat
      if (selectedFriend && message.sender._id === selectedFriend._id) {
        setChatMessages(prev => [...prev, message]);
      }
    });
  };

  // Search users with debouncing
  useEffect(() => {
    if (searchQuery.length >= 2) {
      setIsSearching(true);
      const timeoutId = setTimeout(async () => {
        try {
          const data = await friendsAPI.searchUsers(searchQuery);
          setSearchResults(data.users);
        } catch (error) {
          console.error('Search failed:', error);
          setSearchResults([]);
        } finally {
          setIsSearching(false);
        }
      }, 300);

      return () => clearTimeout(timeoutId);
    } else {
      setSearchResults([]);
      setIsSearching(false);
    }
  }, [searchQuery]);

  // Load chat when friend is selected
  useEffect(() => {
    if (selectedFriend) {
      loadChatHistory();
    }
  }, [selectedFriend]);

  // Auto-scroll to bottom when messages change
  useEffect(() => {
    scrollToBottom();
  }, [chatMessages]);

  const scrollToBottom = () => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  };

  const loadFriends = async () => {
    setIsLoading(true);
    try {
      const data = await friendsAPI.getFriends();
      setFriends(data.friends);
      setPendingRequests(data.pending);
    } catch (error) {
      console.error('Failed to load friends:', error);
    } finally {
      setIsLoading(false);
    }
  };

  const loadChatHistory = async () => {
    if (!selectedFriend) return;
    try {
      const data = await friendsAPI.getChatHistory(selectedFriend._id);
      setChatMessages(data.messages);
    } catch (error) {
      console.error('Failed to load chat:', error);
    }
  };

  const handleSendFriendRequest = async (userId) => {
    try {
      await friendsAPI.sendFriendRequest(userId);
      // Update search results to show new status
      setSearchResults(prev => prev.map(user => 
        user._id === userId ? { ...user, friendshipStatus: 'sent' } : user
      ));
    } catch (error) {
      console.error('Failed to send friend request:', error);
      alert('Failed to send friend request');
    }
  };

  const handleAcceptRequest = async (requestId) => {
    try {
      await friendsAPI.acceptFriendRequest(requestId);
      await loadFriends();
    } catch (error) {
      console.error('Failed to accept request:', error);
      alert('Failed to accept friend request');
    }
  };

  const handleDeclineRequest = async (requestId) => {
    try {
      await friendsAPI.declineFriendRequest(requestId);
      await loadFriends();
    } catch (error) {
      console.error('Failed to decline request:', error);
      alert('Failed to decline friend request');
    }
  };

  const handleSendMessage = async (e) => {
    e.preventDefault();
    if (!newMessage.trim() || !selectedFriend) return;

    try {
      console.log('📤 Sending message from FriendsModal to:', selectedFriend.username, 'Content:', newMessage.trim());
      const message = await friendsAPI.sendMessage(selectedFriend._id, newMessage.trim());
      console.log('✅ Message sent from FriendsModal:', message);
      setChatMessages(prev => [...prev, message]);
      setNewMessage('');
    } catch (error) {
      console.error('❌ Failed to send message:', error);
      alert('Failed to send message');
    }
  };

  const handleStartChat = (friend) => {
    setSelectedFriend(friend);
    setActiveTab('chat');
  };

  if (!isOpen) return null;

  return (
    <div className="friends-modal-overlay" onClick={onClose}>
      <div className="friends-modal" onClick={(e) => e.stopPropagation()}>
        <div className="friends-modal-header">
          <h2>👥 Friends & Chat</h2>
          <button onClick={onClose} className="friends-modal-close">×</button>
        </div>

        {/* Tab Navigation */}
        <div className="friends-tabs">
          <button 
            className={`friends-tab ${activeTab === 'friends' ? 'active' : ''}`}
            onClick={() => setActiveTab('friends')}
          >
            Friends ({friends.length})
          </button>
          <button 
            className={`friends-tab ${activeTab === 'search' ? 'active' : ''}`}
            onClick={() => setActiveTab('search')}
          >
            Add Friends
          </button>
          <button 
            className={`friends-tab ${activeTab === 'requests' ? 'active' : ''}`}
            onClick={() => setActiveTab('requests')}
          >
            Requests ({pendingRequests.received.length})
          </button>
          {selectedFriend && (
            <button 
              className={`friends-tab ${activeTab === 'chat' ? 'active' : ''}`}
              onClick={() => setActiveTab('chat')}
            >
              💬 {selectedFriend.username}
            </button>
          )}
        </div>

        <div className="friends-content">
          {/* Friends List */}
          {activeTab === 'friends' && (
            <div className="friends-list">
              {isLoading ? (
                <div className="friends-loading">Loading friends...</div>
              ) : friends.length === 0 ? (
                <div className="friends-empty">
                  <p>No friends yet! Search for users to add them as friends.</p>
                </div>
              ) : (
                <div className="friends-grid">
                  {friends.map(friend => (
                    <div key={friend._id} className="friend-card">
                      <div className="friend-info">
                        <h4>{friend.username}</h4>
                        <p>{friend.email}</p>
                      </div>
                      <div className="friend-actions">
                        <button 
                          onClick={() => handleStartChat(friend)}
                          className="btn-primary btn-sm"
                        >
                          💬 Chat
                        </button>
                      </div>
                    </div>
                  ))}
                </div>
              )}
            </div>
          )}

          {/* Search for Friends */}
          {activeTab === 'search' && (
            <div className="friends-search">
              <div className="search-input-container">
                <input
                  type="text"
                  placeholder="Search by username or email..."
                  value={searchQuery}
                  onChange={(e) => setSearchQuery(e.target.value)}
                  className="search-input"
                />
                {isSearching && <div className="search-spinner">🔍</div>}
              </div>

              {searchQuery.length < 2 ? (
                <div className="search-hint">
                  Type at least 2 characters to search for users
                </div>
              ) : (
                <div className="search-results">
                  {searchResults.length === 0 && !isSearching ? (
                    <div className="search-empty">No users found</div>
                  ) : (
                    searchResults.map(user => (
                      <div key={user._id} className="search-result">
                        <div className="user-info">
                          <h4>{user.username}</h4>
                          <p>{user.email}</p>
                        </div>
                        <div className="user-actions">
                          {user.friendshipStatus === 'none' && (
                            <button 
                              onClick={() => handleSendFriendRequest(user._id)}
                              className="btn-primary btn-sm"
                            >
                              ➕ Add Friend
                            </button>
                          )}
                          {user.friendshipStatus === 'sent' && (
                            <span className="status-badge sent">Request Sent</span>
                          )}
                          {user.friendshipStatus === 'received' && (
                            <span className="status-badge received">Pending</span>
                          )}
                          {user.friendshipStatus === 'friends' && (
                            <span className="status-badge friends">✅ Friends</span>
                          )}
                        </div>
                      </div>
                    ))
                  )}
                </div>
              )}
            </div>
          )}

          {/* Friend Requests */}
          {activeTab === 'requests' && (
            <div className="friend-requests">
              <div className="requests-section">
                <h3>Received Requests ({pendingRequests.received.length})</h3>
                {pendingRequests.received.length === 0 ? (
                  <p>No pending friend requests</p>
                ) : (
                  <div className="requests-list">
                    {pendingRequests.received.map(request => (
                      <div key={request._id} className="request-item">
                        <div className="request-info">
                          <h4>{request.user.username}</h4>
                          <p>{request.user.email}</p>
                        </div>
                        <div className="request-actions">
                          <button 
                            onClick={() => handleAcceptRequest(request._id)}
                            className="btn-primary btn-sm"
                          >
                            ✅ Accept
                          </button>
                          <button 
                            onClick={() => handleDeclineRequest(request._id)}
                            className="btn-secondary btn-sm"
                          >
                            ❌ Decline
                          </button>
                        </div>
                      </div>
                    ))}
                  </div>
                )}
              </div>

              <div className="requests-section">
                <h3>Sent Requests ({pendingRequests.sent.length})</h3>
                {pendingRequests.sent.length === 0 ? (
                  <p>No outgoing friend requests</p>
                ) : (
                  <div className="requests-list">
                    {pendingRequests.sent.map(request => (
                      <div key={request._id} className="request-item">
                        <div className="request-info">
                          <h4>{request.user.username}</h4>
                          <p>{request.user.email}</p>
                        </div>
                        <div className="request-status">
                          <span className="status-badge sent">Pending</span>
                        </div>
                      </div>
                    ))}
                  </div>
                )}
              </div>
            </div>
          )}

          {/* Chat Interface */}
          {activeTab === 'chat' && selectedFriend && (
            <div className="chat-interface">
              <div className="chat-header">
                <h3>💬 Chat with {selectedFriend.username}</h3>
              </div>
              
              <div className="chat-messages">
                {chatMessages.length === 0 ? (
                  <div className="chat-empty">
                    Start a conversation with {selectedFriend.username}!
                  </div>
                ) : (
                  chatMessages.map(message => (
                    <div 
                      key={message._id} 
                      className={`message ${message.sender._id === selectedFriend._id ? 'received' : 'sent'}`}
                    >
                      <div className="message-content">
                        {message.messageType === 'room_invite' ? (
                          <div className="room-invite-message">
                            🏠 <strong>Room Invitation:</strong> {message.roomInvite.roomName}
                            <button className="btn-primary btn-sm">Join Room</button>
                          </div>
                        ) : (
                          <p>{message.content}</p>
                        )}
                      </div>
                      <div className="message-time">
                        {new Date(message.createdAt).toLocaleTimeString()}
                      </div>
                    </div>
                  ))
                )}
                <div ref={messagesEndRef} />
              </div>

              <form onSubmit={handleSendMessage} className="chat-input-form">
                <input
                  type="text"
                  value={newMessage}
                  onChange={(e) => setNewMessage(e.target.value)}
                  placeholder="Type a message..."
                  className="chat-input"
                />
                <button type="submit" className="btn-primary">Send</button>
              </form>
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
