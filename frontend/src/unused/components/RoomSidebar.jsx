import React, { useState, useEffect } from 'react';
import { socket } from '../lib/socket';
import { getCurrentUser } from '../lib/auth';

export default function RoomSidebar({ room, isOpen, onToggle }) {
  const [activeTab, setActiveTab] = useState('chat');
  const [onlineMembers, setOnlineMembers] = useState(new Set());
  const [memberDetails, setMemberDetails] = useState({});
  const [messages, setMessages] = useState([]);
  const [newMessage, setNewMessage] = useState('');
  const currentUser = getCurrentUser();

  // Socket listeners for online status
  useEffect(() => {
    if (!room) return; // Remove the isOpen condition

    console.log('🔌 Setting up socket listeners for room:', room._id);
    console.log('👤 Current user:', currentUser);
    console.log('🔌 Socket connected?', socket.connected);
    console.log('🔌 Socket ID:', socket.id);

    // Add current user to online members initially if socket is connected
    if (socket.connected && currentUser?.userId) {
      console.log('➕ Adding current user to online members:', currentUser.userId);
      setOnlineMembers(prev => new Set([...prev, currentUser.userId]));
    }

    const handleJoinedRoom = (data) => {
      console.log('🏠 Joined room event:', data);
      if (data.onlineUsers) {
        const onlineUserIds = data.onlineUsers.map(user => user.userId);
        console.log('✅ Online user IDs:', onlineUserIds);
        setOnlineMembers(new Set(onlineUserIds));
      }
      
      // Load existing messages from database
      if (data.messages && Array.isArray(data.messages)) {
        console.log('📜 Loading message history:', data.messages.length, 'messages');
        const formattedMessages = data.messages.map(msg => ({
          id: msg.messageId || Date.now() + Math.random(),
          type: 'message',
          username: msg.username,
          text: msg.message,
          timestamp: new Date(msg.timestamp),
          userId: msg.userId
        }));
        setMessages(formattedMessages);
      }
    };

    const handleUserJoined = (data) => {
      console.log('👋 User joined event:', data);
      if (data.roomId === room._id) {
        setOnlineMembers(prev => {
          const newSet = new Set([...prev, data.userId]);
          console.log('📊 Updated online members after join:', Array.from(newSet));
          return newSet;
        });
        setMessages(prev => [...prev, {
          id: Date.now(),
          type: 'system',
          text: `${data.username || data.userEmail} joined the room`,
          timestamp: new Date()
        }]);
      }
    };

    const handleUserLeft = (data) => {
      console.log('👋 User left event:', data);
      if (data.roomId === room._id) {
        setOnlineMembers(prev => {
          const newSet = new Set(prev);
          newSet.delete(data.userId);
          console.log('📊 Updated online members after leave:', Array.from(newSet));
          return newSet;
        });
        setMessages(prev => [...prev, {
          id: Date.now(),
          type: 'system',
          text: `${data.username || data.userEmail} left the room`,
          timestamp: new Date()
        }]);
      }
    };

    const handleRoomMessage = (data) => {
      console.log('📥 Received room message:', data);
      if (data.roomId === room._id) {
        const newMessage = {
          id: Date.now(),
          type: 'message',
          username: data.username,
          text: data.message,
          timestamp: new Date(),
          userId: data.userId
        };
        console.log('💬 Adding message to chat:', newMessage);
        setMessages(prev => [...prev, newMessage]);
      }
    };

    socket.on('joined-room', handleJoinedRoom);
    socket.on('user-joined-room', handleUserJoined);
    socket.on('user-left-room', handleUserLeft);
    socket.on('room-message', handleRoomMessage);

    // Test socket connection
    socket.emit('test-connection', { roomId: room._id, userId: currentUser?.userId });
    console.log('🧪 Sent test connection event');

    return () => {
      socket.off('joined-room', handleJoinedRoom);
      socket.off('user-joined-room', handleUserJoined);
      socket.off('user-left-room', handleUserLeft);
      socket.off('room-message', handleRoomMessage);
    };
  }, [room?._id]); // Remove isOpen dependency

  // Setup member details
  useEffect(() => {
    if (!room?.members) return;
    
    console.log('🔍 Setting up member details for room:', room.name);
    console.log('📋 Raw room members:', room.members);
    
    const details = {};
    room.members.forEach(member => {
      console.log('👤 Processing member:', member);
      // Handle both data structures - populated and non-populated
      const userId = member.user?._id || member.user || member.userId;
      const userEmail = member.user?.email || member.userEmail || member.email;
      const username = member.user?.username || member.username || userEmail?.split('@')[0];
      
      details[userId] = {
        email: userEmail || 'Unknown',
        username: username || 'Unknown',
        role: member.role || 'member',
        joinedAt: member.joinedAt
      };
    });
    
    console.log('📝 Final member details:', details);
    setMemberDetails(details);
  }, [room?.members]);

  const sendMessage = (e) => {
    e.preventDefault();
    if (!newMessage.trim()) return;

    const messageData = {
      roomId: room._id,
      message: newMessage.trim(),
      username: currentUser?.username || currentUser?.email?.split('@')[0] || 'Anonymous',
      userId: currentUser?.userId
    };

    console.log('📤 Sending message:', messageData);
    console.log('🔌 Socket connected?', socket.connected);
    console.log('🔌 Socket ID:', socket.id);

    if (!socket.connected) {
      console.error('❌ Socket not connected, cannot send message');
      // Add a local error message
      setMessages(prev => [...prev, {
        id: Date.now(),
        type: 'system',
        text: 'Connection lost. Please refresh the page.',
        timestamp: new Date()
      }]);
      return;
    }

    console.log('📤 Sending message via socket:', messageData);
    socket.emit('room-message', messageData);
    setNewMessage('');
  };

  const formatTime = (date) => {
    return date.toLocaleTimeString('en-US', {
      hour: '2-digit',
      minute: '2-digit',
      hour12: false
    });
  };

  const getRoleIcon = (role) => {
    switch (role) {
      case 'owner': return '👑';
      case 'admin': return '🛡️';
      default: return '👤';
    }
  };

  const getRoleColor = (role) => {
    switch (role) {
      case 'owner': return 'text-yellow-400';
      case 'admin': return 'text-blue-400';
      default: return 'text-gray-300';
    }
  };

  if (!room) return null;

  return (
    <>
      {/* Sidebar - Full Height */}
      <div className={`fixed top-0 right-0 h-screen transition-transform duration-300 z-50 shadow-2xl ${
        isOpen ? 'translate-x-0' : 'translate-x-full'
      }`} style={{ 
        width: '350px', 
        pointerEvents: 'auto',
        background: 'linear-gradient(180deg, rgb(17, 24, 39) 0%, rgb(31, 41, 55) 50%, rgb(17, 24, 39) 100%)',
        borderLeft: '1px solid rgb(75, 85, 99)'
      }}>
        
        {/* Header */}
        <div className="h-16 px-4 border-b border-gray-600 flex items-center" style={{ 
          background: 'rgba(31, 41, 55, 0.95)',
          backdropFilter: 'blur(10px)'
        }}>
          <div className="flex-1">
            <h3 className="text-lg font-semibold text-white">{room.name}</h3>
            <p className="text-xs text-gray-300">
              {onlineMembers.size} online • {room.members?.length || 0} total
            </p>
          </div>
        </div>

        {/* Tab Navigation */}
        <div className="px-4 py-3 border-b border-gray-600" style={{ 
          background: 'rgba(31, 41, 55, 0.8)',
          backdropFilter: 'blur(10px)'
        }}>
          <div className="flex rounded-lg bg-gray-800 p-1">
            <button
              onClick={() => setActiveTab('chat')}
              className={`flex-1 py-3 px-4 rounded-md text-sm font-medium transition-all duration-200 ${
                activeTab === 'chat'
                  ? 'bg-blue-600 text-white shadow-lg'
                  : 'text-gray-400 hover:text-gray-200 hover:bg-gray-700'
              }`}
            >
              💬 Chat
            </button>
            <button
              onClick={() => setActiveTab('members')}
              className={`flex-1 py-3 px-4 rounded-md text-sm font-medium transition-all duration-200 ${
                activeTab === 'members'
                  ? 'bg-blue-600 text-white shadow-lg'
                  : 'text-gray-400 hover:text-gray-200 hover:bg-gray-700'
              }`}
            >
              👥 Members
            </button>
          </div>
        </div>

        {/* Content Area - Takes remaining height */}
        <div className="flex flex-col" style={{ height: 'calc(100vh - 120px)' }}>
          {activeTab === 'chat' ? (
            <>
              {/* Chat Messages */}
              <div className="flex-1 overflow-y-auto p-4" style={{ 
                background: 'rgba(17, 24, 39, 0.8)',
                backdropFilter: 'blur(5px)'
              }}>
                {messages.length === 0 ? (
                  <div className="text-center text-gray-400 mt-8">
                    <div className="text-4xl mb-2">💬</div>
                    <p className="text-white">No messages yet</p>
                    <p className="text-sm mt-1 text-gray-500">Start the conversation!</p>
                  </div>
                ) : (
                  <div className="">
                    {messages.map((msg, index) => {
                      const prevMessage = index > 0 ? messages[index - 1] : null;
                      const isGrouped = prevMessage && 
                                       prevMessage.type === 'message' && 
                                       msg.type === 'message' && 
                                       prevMessage.userId === msg.userId &&
                                       (new Date(msg.timestamp) - new Date(prevMessage.timestamp)) < 60000; // Within 1 minute
                      
                      return (
                        <div key={`${msg.id}-${index}`} className={`${
                          msg.type === 'system' 
                            ? 'text-center text-gray-500 text-xs italic' 
                            : ``
                        }`}>
                          {msg.type === 'system' ? (
                            <span>• {msg.text}</span>
                          ) : (
                            <div className={`${isGrouped ? '' : 'bg-gray-800 rounded border border-gray-700'}`}>
                              {!isGrouped && (
                                <div className="px-2 pt-1">
                                  <span className="font-semibold text-blue-400 text-xs">{msg.username}</span>
                                </div>
                              )}
                              <span className={`${isGrouped ? 'bg-gray-800 rounded border border-gray-700 mx-4 px-2 block' : 'px-2 block'} text-gray-200 text-xs leading-tight`}>{msg.text}</span>
                            </div>
                          )}
                        </div>
                      );
                    })}
                  </div>
                )}
              </div>

              {/* Message Input */}
              <div className="p-4 border-t border-gray-600" style={{ 
                background: 'rgba(31, 41, 55, 0.95)',
                backdropFilter: 'blur(10px)',
                position: 'relative', 
                zIndex: 51 
              }}>
                <form onSubmit={sendMessage} className="flex gap-2">
                  <input
                    type="text"
                    value={newMessage}
                    onChange={(e) => setNewMessage(e.target.value)}
                    placeholder="Type a message..."
                    className="flex-1 bg-gray-800 text-white rounded-lg px-3 py-2.5 text-sm border border-gray-600 focus:border-blue-500 focus:outline-none focus:ring-1 focus:ring-blue-500"
                    style={{ pointerEvents: 'auto' }}
                  />
                  <button
                    type="submit"
                    disabled={!newMessage.trim()}
                    className="bg-blue-600 text-white rounded-lg px-4 py-2.5 text-sm font-medium hover:bg-blue-700 disabled:opacity-50 disabled:cursor-not-allowed transition-all duration-200"
                    style={{ pointerEvents: 'auto' }}
                  >
                    Send
                  </button>
                </form>
              </div>
            </>
          ) : (
            /* Members List */
            <div className="flex-1 overflow-y-auto p-4" style={{ 
              background: 'rgba(17, 24, 39, 0.8)',
              backdropFilter: 'blur(5px)'
            }}>
              <div className="space-y-3">
                {room.members
                  ?.sort((a, b) => {
                    const aUserId = a.user?._id || a.user || a.userId;
                    const bUserId = b.user?._id || b.user || b.userId;
                    const aOnline = onlineMembers.has(aUserId);
                    const bOnline = onlineMembers.has(bUserId);
                    if (aOnline && !bOnline) return -1;
                    if (!aOnline && bOnline) return 1;
                    
                    const roleOrder = { owner: 0, admin: 1, member: 2 };
                    const aRole = memberDetails[aUserId]?.role || 'member';
                    const bRole = memberDetails[bUserId]?.role || 'member';
                    return roleOrder[aRole] - roleOrder[bRole];
                  })
                  .map((member) => {
                    const userId = member.user?._id || member.user || member.userId;
                    const isOnline = onlineMembers.has(userId);
                    const details = memberDetails[userId];
                    
                    return (
                      <div
                        key={userId}
                        className={`flex items-center gap-3 p-3 rounded-lg border transition-all duration-200 ${
                          isOnline 
                            ? 'bg-gray-800 border-gray-600 shadow-md' 
                            : 'border-gray-700'
                        }`}
                        style={{ backgroundColor: isOnline ? undefined : 'rgb(31, 41, 55)' }}
                      >
                        <div className="relative">
                          <div className={`w-10 h-10 rounded-full flex items-center justify-center text-white text-sm font-bold ${
                            isOnline
                              ? 'bg-gradient-to-br from-blue-500 to-purple-600'
                              : 'bg-gradient-to-br from-gray-600 to-gray-700'
                          }`}>
                            {(details?.username || details?.email || 'U').charAt(0).toUpperCase()}
                          </div>
                          <div className={`absolute -bottom-1 -right-1 w-3 h-3 rounded-full border-2 border-gray-900 ${
                            isOnline ? 'bg-green-500' : 'bg-gray-500'
                          }`} />
                        </div>
                        
                        <div className="flex-1 min-w-0">
                          <div className="flex items-center gap-2 mb-1">
                            <span className="text-sm">{getRoleIcon(details?.role)}</span>
                            <span className={`text-xs font-medium ${getRoleColor(details?.role)}`}>
                              {details?.role || 'member'}
                            </span>
                          </div>
                          <p className="text-white text-sm font-medium truncate">
                            {details?.username || details?.email?.split('@')[0] || 'Unknown'}
                          </p>
                          <p className={`text-xs ${isOnline ? 'text-green-400' : 'text-gray-500'}`}>
                            {isOnline ? 'Online' : 'Offline'}
                          </p>
                        </div>
                      </div>
                    );
                  })}
              </div>
              
              {/* Members Stats */}
              <div className="mt-4 p-3 bg-gray-800 rounded-lg border border-gray-700">
                <div className="text-center">
                  <div className="flex justify-between text-sm mb-1">
                    <span className="text-gray-400">Online:</span>
                    <span className="text-green-400 font-semibold">{onlineMembers.size}</span>
                  </div>
                  <div className="flex justify-between text-sm">
                    <span className="text-gray-400">Total:</span>
                    <span className="text-white font-semibold">{room.members?.length || 0}</span>
                  </div>
                </div>
              </div>
            </div>
          )}
        </div>
      </div>

      {/* Backdrop */}
      {isOpen && (
        <div 
          className="fixed inset-0 bg-black bg-opacity-30 z-30 backdrop-blur-sm"
          onClick={onToggle}
          style={{ pointerEvents: 'auto' }}
        />
      )}
    </>
  );
}
