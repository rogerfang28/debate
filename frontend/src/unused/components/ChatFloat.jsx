import { useState, useEffect, useRef } from 'react';
import { friendsAPI } from '../lib/friendsAPI';
import { getCurrentUser } from '../lib/auth';
import { socket } from '../lib/socket';
import './ChatFloat.css';

export function ChatFloat() {
  const [isOpen, setIsOpen] = useState(false);
  const [openChats, setOpenChats] = useState(new Map());
  const [friends, setFriends] = useState([]);
  const [unreadCounts, setUnreadCounts] = useState(new Map());
  const [pendingRequestsCount, setPendingRequestsCount] = useState(0);
  const [isLoadingFriends, setIsLoadingFriends] = useState(false);
  const currentUser = getCurrentUser();

  // Debug socket connection
  useEffect(() => {
    console.log('💬 ChatFloat: Socket connected?', socket.connected);
    console.log('💬 ChatFloat: Current user:', currentUser);
  }, [currentUser]);

  useEffect(() => {
    if (currentUser && currentUser.userId) {
      console.log('💬 ChatFloat: Setting up for user', currentUser.userId);
      loadFriends();
      setupSocketListeners();
    }

    return () => {
      console.log('💬 ChatFloat: Cleaning up socket listeners');
      // Clean up socket listeners
      socket.off('new-message-chatfloat');
      socket.off('friend-request-received-chatfloat');
      socket.off('friend-request-accepted-chatfloat');
    };
  }, [currentUser?.userId]); // Only depend on userId, not the whole object

  const setupSocketListeners = () => {
    // Use unique event names to prevent conflicts with FriendsModal
    // Remove existing listeners to prevent duplicates
    socket.off('new-message-chatfloat');
    socket.off('friend-request-received-chatfloat');
    socket.off('friend-request-accepted-chatfloat');

    console.log('💬 ChatFloat: Setting up socket listeners');

    socket.on('new-message-chatfloat', (message) => {
      console.log('📨 New message received in ChatFloat:', message);
      const senderId = message.sender._id;
      
      // Update unread count if chat is not open
      const chatKey = senderId;
      if (!openChats.has(chatKey)) {
        setUnreadCounts(prev => new Map(prev.set(senderId, (prev.get(senderId) || 0) + 1)));
      } else {
        // Add message to open chat
        setOpenChats(prev => {
          const newChats = new Map(prev);
          const chat = newChats.get(chatKey);
          if (chat) {
            newChats.set(chatKey, {
              ...chat,
              messages: [...chat.messages, message]
            });
          }
          return newChats;
        });
      }
    });

    socket.on('friend-request-received-chatfloat', () => {
      console.log('👥 Friend request received in ChatFloat');
      setPendingRequestsCount(prev => prev + 1);
      // Don't reload friends here to prevent loop
    });

    socket.on('friend-request-accepted-chatfloat', () => {
      console.log('✅ Friend request accepted in ChatFloat');
      // Don't reload friends here to prevent loop
    });
  };

  const loadFriends = async () => {
    if (isLoadingFriends || !currentUser?.userId) {
      console.log('💬 ChatFloat: Skipping loadFriends - already loading or no user');
      return;
    }
    setIsLoadingFriends(true);
    try {
      console.log('📱 Loading friends in ChatFloat...');
      const data = await friendsAPI.getFriends();
      setFriends(data.friends);
      setPendingRequestsCount(data.pending.received.length);
      console.log('✅ Friends loaded:', data.friends.length, 'Pending:', data.pending.received.length);
    } catch (error) {
      console.error('❌ Failed to load friends:', error);
    } finally {
      setIsLoadingFriends(false);
    }
  };

  const openChat = async (friend) => {
    const chatKey = friend._id;
    
    if (!openChats.has(chatKey)) {
      try {
        console.log('💬 Opening chat with:', friend.username);
        const data = await friendsAPI.getChatHistory(friend._id, 1, 20);
        setOpenChats(prev => new Map(prev.set(chatKey, {
          friend,
          messages: data.messages,
          newMessage: ''
        })));
        
        // Clear unread count
        setUnreadCounts(prev => {
          const newCounts = new Map(prev);
          newCounts.delete(friend._id);
          return newCounts;
        });
        console.log('✅ Chat opened successfully');
      } catch (error) {
        console.error('❌ Failed to load chat:', error);
      }
    }
  };

  const closeChat = (chatKey) => {
    setOpenChats(prev => {
      const newChats = new Map(prev);
      newChats.delete(chatKey);
      return newChats;
    });
  };

  const sendMessage = async (friendId, content) => {
    if (!content.trim()) return;

    try {
      console.log('📤 Sending message to:', friendId, 'Content:', content.trim());
      const message = await friendsAPI.sendMessage(friendId, content.trim());
      console.log('✅ Message sent:', message);
      
      // Add message to chat
      setOpenChats(prev => {
        const newChats = new Map(prev);
        const chat = newChats.get(friendId);
        if (chat) {
          newChats.set(friendId, {
            ...chat,
            messages: [...chat.messages, message],
            newMessage: ''
          });
        }
        return newChats;
      });
    } catch (error) {
      console.error('❌ Failed to send message:', error);
      alert('Failed to send message. Please try again.');
    }
  };

  const updateNewMessage = (friendId, message) => {
    setOpenChats(prev => {
      const newChats = new Map(prev);
      const chat = newChats.get(friendId);
      if (chat) {
        newChats.set(friendId, { ...chat, newMessage: message });
      }
      return newChats;
    });
  };

  const totalUnreadCount = Array.from(unreadCounts.values()).reduce((sum, count) => sum + count, 0);
  const totalNotifications = totalUnreadCount + pendingRequestsCount;

  return (
    <div className="chat-float-container">
      {/* Open Chat Windows */}
      <div className="chat-windows">
        {Array.from(openChats.entries()).map(([friendId, chat]) => (
          <ChatWindow
            key={friendId}
            chat={chat}
            onClose={() => closeChat(friendId)}
            onSendMessage={(content) => sendMessage(friendId, content)}
            onUpdateMessage={(message) => updateNewMessage(friendId, message)}
          />
        ))}
      </div>

      {/* Main Chat Button */}
      <div className="chat-float-button" onClick={() => setIsOpen(!isOpen)}>
        💬
        {totalNotifications > 0 && (
          <div className="notification-badge">
            {totalNotifications > 99 ? '99+' : totalNotifications}
          </div>
        )}
      </div>

      {/* Friends List Popup */}
      {isOpen && (
        <div className="chat-float-popup">
          <div className="chat-float-header">
            <h3>👥 Friends & Chat</h3>
            <button onClick={() => setIsOpen(false)}>×</button>
          </div>
          
          {pendingRequestsCount > 0 && (
            <div className="pending-requests-notice">
              <span>🔔 {pendingRequestsCount} pending friend request{pendingRequestsCount !== 1 ? 's' : ''}</span>
            </div>
          )}

          <div className="friends-quick-list">
            {isLoadingFriends ? (
              <div className="loading">Loading friends...</div>
            ) : friends.length === 0 ? (
              <div className="no-friends">No friends yet</div>
            ) : (
              friends.map(friend => (
                <div 
                  key={friend._id} 
                  className="friend-quick-item"
                  onClick={() => {
                    openChat(friend);
                    setIsOpen(false);
                  }}
                >
                  <div className="friend-info">
                    <strong>{friend.username}</strong>
                    {unreadCounts.has(friend._id) && (
                      <span className="unread-badge">
                        {unreadCounts.get(friend._id)}
                      </span>
                    )}
                  </div>
                </div>
              ))
            )}
          </div>
        </div>
      )}
    </div>
  );
}

function ChatWindow({ chat, onClose, onSendMessage, onUpdateMessage }) {
  const messagesEndRef = useRef(null);

  const scrollToBottom = () => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  };

  useEffect(() => {
    scrollToBottom();
  }, [chat.messages]);

  const handleSubmit = (e) => {
    e.preventDefault();
    if (chat.newMessage.trim()) {
      onSendMessage(chat.newMessage);
    }
  };

  return (
    <div className="chat-window">
      <div className="chat-window-header">
        <span>{chat.friend.username}</span>
        <button onClick={onClose}>×</button>
      </div>
      
      <div className="chat-window-messages">
        {chat.messages.map(message => (
          <div 
            key={message._id} 
            className={`message ${message.sender._id === chat.friend._id ? 'received' : 'sent'}`}
          >
            <div className="message-content">
              {message.messageType === 'room_invite' ? (
                <div className="room-invite-message">
                  🏠 <strong>Room Invitation:</strong> {message.roomInvite.roomName}
                </div>
              ) : (
                <p>{message.content}</p>
              )}
            </div>
            <div className="message-time">
              {new Date(message.createdAt).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}
            </div>
          </div>
        ))}
        <div ref={messagesEndRef} />
      </div>

      <form onSubmit={handleSubmit} className="chat-window-input">
        <input
          type="text"
          value={chat.newMessage}
          onChange={(e) => onUpdateMessage(e.target.value)}
          placeholder="Type a message..."
          autoFocus
        />
        <button type="submit">Send</button>
      </form>
    </div>
  );
}
