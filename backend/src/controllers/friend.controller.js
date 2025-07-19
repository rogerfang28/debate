import { FriendModel } from '../models/Friend.js';
import { MessageModel } from '../models/Message.js';
import { UserModel } from '../models/User.js';
import { RoomModel } from '../models/Room.js';
import { emitToUser } from '../lib/socketHandler.js';

export const friendController = {
  // Search for users to add as friends
  async searchUsers(req, res) {
    try {
      const { query } = req.query;
      const currentUserId = req.user.userId;

      if (!query || query.length < 2) {
        return res.status(400).json({ 
          message: 'Search query must be at least 2 characters' 
        });
      }

      // Find users by username or email (case-insensitive)
      const users = await UserModel.find({
        _id: { $ne: currentUserId }, // Exclude current user
        $or: [
          { username: { $regex: query, $options: 'i' } },
          { email: { $regex: query, $options: 'i' } }
        ]
      }).select('username email').limit(10);

      // Get existing friend relationships to show status
      const userIds = users.map(user => user._id);
      const friendships = await FriendModel.find({
        $or: [
          { requester: currentUserId, recipient: { $in: userIds } },
          { requester: { $in: userIds }, recipient: currentUserId }
        ]
      });

      // Add friendship status to each user
      const usersWithStatus = users.map(user => {
        const friendship = friendships.find(f => 
          (f.requester.toString() === currentUserId && f.recipient.toString() === user._id.toString()) ||
          (f.recipient.toString() === currentUserId && f.requester.toString() === user._id.toString())
        );

        let status = 'none';
        if (friendship) {
          if (friendship.status === 'accepted') {
            status = 'friends';
          } else if (friendship.requester.toString() === currentUserId) {
            status = 'sent';
          } else {
            status = 'received';
          }
        }

        return {
          _id: user._id,
          username: user.username,
          email: user.email,
          friendshipStatus: status
        };
      });

      res.json({ users: usersWithStatus });
    } catch (error) {
      console.error('Search users error:', error);
      res.status(500).json({ message: 'Failed to search users' });
    }
  },

  // Send friend request
  async sendFriendRequest(req, res) {
    try {
      const { userId } = req.params;
      const currentUserId = req.user.userId;

      if (userId === currentUserId) {
        return res.status(400).json({ message: 'Cannot send friend request to yourself' });
      }

      // Check if user exists
      const targetUser = await UserModel.findById(userId);
      if (!targetUser) {
        return res.status(404).json({ message: 'User not found' });
      }

      // Check if friendship already exists
      const existingFriendship = await FriendModel.findOne({
        $or: [
          { requester: currentUserId, recipient: userId },
          { requester: userId, recipient: currentUserId }
        ]
      });

      if (existingFriendship) {
        return res.status(400).json({ 
          message: 'Friend request already exists or users are already friends' 
        });
      }

      // Create friend request
      const friendRequest = new FriendModel({
        requester: currentUserId,
        recipient: userId
      });

      await friendRequest.save();

      // Emit real-time notification to recipient
      const requester = await UserModel.findById(currentUserId).select('username email');
      
      // Emit to all components
      const eventData = {
        requestId: friendRequest._id,
        from: {
          _id: currentUserId,
          username: requester.username,
          email: requester.email
        }
      };
      
      emitToUser(userId, 'friend-request-received', eventData);
      emitToUser(userId, 'friend-request-received-chatfloat', eventData);
      emitToUser(userId, 'friend-request-received-modal', eventData);
      emitToUser(userId, 'friend-request-received-homepage', eventData);

      res.status(201).json({ 
        message: 'Friend request sent successfully',
        requestId: friendRequest._id
      });
    } catch (error) {
      console.error('Send friend request error:', error);
      res.status(500).json({ message: 'Failed to send friend request' });
    }
  },

  // Accept friend request
  async acceptFriendRequest(req, res) {
    try {
      const { requestId } = req.params;
      const currentUserId = req.user.userId;

      const friendRequest = await FriendModel.findOne({
        _id: requestId,
        recipient: currentUserId,
        status: 'pending'
      });

      if (!friendRequest) {
        return res.status(404).json({ message: 'Friend request not found' });
      }

      friendRequest.status = 'accepted';
      await friendRequest.save();

      // Emit real-time notifications
      const accepter = await UserModel.findById(currentUserId).select('username email');
      
      // Emit to all components
      const eventData = {
        requestId: friendRequest._id,
        by: {
          _id: currentUserId,
          username: accepter.username,
          email: accepter.email
        }
      };
      
      emitToUser(friendRequest.requester.toString(), 'friend-request-accepted', eventData);
      emitToUser(friendRequest.requester.toString(), 'friend-request-accepted-chatfloat', eventData);
      emitToUser(friendRequest.requester.toString(), 'friend-request-accepted-modal', eventData);
      emitToUser(friendRequest.requester.toString(), 'friend-request-accepted-homepage', eventData);

      res.json({ message: 'Friend request accepted' });
    } catch (error) {
      console.error('Accept friend request error:', error);
      res.status(500).json({ message: 'Failed to accept friend request' });
    }
  },

  // Decline friend request
  async declineFriendRequest(req, res) {
    try {
      const { requestId } = req.params;
      const currentUserId = req.user.userId;

      const friendRequest = await FriendModel.findOne({
        _id: requestId,
        recipient: currentUserId,
        status: 'pending'
      });

      if (!friendRequest) {
        return res.status(404).json({ message: 'Friend request not found' });
      }

      friendRequest.status = 'declined';
      await friendRequest.save();

      res.json({ message: 'Friend request declined' });
    } catch (error) {
      console.error('Decline friend request error:', error);
      res.status(500).json({ message: 'Failed to decline friend request' });
    }
  },

  // Get friends list and pending requests
  async getFriends(req, res) {
    try {
      const currentUserId = req.user.userId;

      // Get accepted friends
      const acceptedFriends = await FriendModel.find({
        $or: [
          { requester: currentUserId, status: 'accepted' },
          { recipient: currentUserId, status: 'accepted' }
        ]
      }).populate('requester recipient', 'username email');

      // Get pending requests (both sent and received)
      const pendingRequests = await FriendModel.find({
        $or: [
          { requester: currentUserId, status: 'pending' },
          { recipient: currentUserId, status: 'pending' }
        ]
      }).populate('requester recipient', 'username email');

      // Format friends list
      const friends = acceptedFriends.map(friendship => {
        const friend = friendship.requester._id.toString() === currentUserId ? 
          friendship.recipient : friendship.requester;
        return {
          _id: friend._id,
          username: friend.username,
          email: friend.email,
          friendshipId: friendship._id
        };
      });

      // Format pending requests
      const sent = pendingRequests
        .filter(req => req.requester._id.toString() === currentUserId)
        .map(req => ({
          _id: req._id,
          user: {
            _id: req.recipient._id,
            username: req.recipient.username,
            email: req.recipient.email
          }
        }));

      const received = pendingRequests
        .filter(req => req.recipient._id.toString() === currentUserId)
        .map(req => ({
          _id: req._id,
          user: {
            _id: req.requester._id,
            username: req.requester.username,
            email: req.requester.email
          }
        }));

      res.json({
        friends,
        pending: { sent, received }
      });
    } catch (error) {
      console.error('Get friends error:', error);
      res.status(500).json({ message: 'Failed to get friends list' });
    }
  },

  // Remove friend
  async removeFriend(req, res) {
    try {
      const { friendshipId } = req.params;
      const currentUserId = req.user.userId;

      const friendship = await FriendModel.findOne({
        _id: friendshipId,
        $or: [
          { requester: currentUserId },
          { recipient: currentUserId }
        ],
        status: 'accepted'
      });

      if (!friendship) {
        return res.status(404).json({ message: 'Friendship not found' });
      }

      await FriendModel.deleteOne({ _id: friendshipId });

      res.json({ message: 'Friend removed successfully' });
    } catch (error) {
      console.error('Remove friend error:', error);
      res.status(500).json({ message: 'Failed to remove friend' });
    }
  },

  // Send message to friend
  async sendMessage(req, res) {
    try {
      const { friendId } = req.params;
      const { content, messageType = 'text', roomInvite } = req.body;
      const currentUserId = req.user.userId;

      if (!content || content.trim().length === 0) {
        return res.status(400).json({ message: 'Message content is required' });
      }

      // Verify friendship exists
      const friendship = await FriendModel.findOne({
        $or: [
          { requester: currentUserId, recipient: friendId },
          { requester: friendId, recipient: currentUserId }
        ],
        status: 'accepted'
      });

      if (!friendship) {
        return res.status(403).json({ message: 'You can only message friends' });
      }

      // Create message
      const message = new MessageModel({
        sender: currentUserId,
        recipient: friendId,
        content: content.trim(),
        messageType,
        ...(roomInvite && { roomInvite })
      });

      await message.save();
      await message.populate('sender', 'username email');

      // Emit real-time message to recipient
      // Emit to both ChatFloat and FriendsModal
      emitToUser(friendId, 'new-message', message);
      emitToUser(friendId, 'new-message-chatfloat', message);
      emitToUser(friendId, 'new-message-modal', message);

      res.status(201).json(message);
    } catch (error) {
      console.error('Send message error:', error);
      res.status(500).json({ message: 'Failed to send message' });
    }
  },

  // Get chat history with a friend
  async getChatHistory(req, res) {
    try {
      const { friendId } = req.params;
      const { page = 1, limit = 50 } = req.query;
      const currentUserId = req.user.userId;

      // Verify friendship exists
      const friendship = await FriendModel.findOne({
        $or: [
          { requester: currentUserId, recipient: friendId },
          { requester: friendId, recipient: currentUserId }
        ],
        status: 'accepted'
      });

      if (!friendship) {
        return res.status(403).json({ message: 'You can only view chat with friends' });
      }

      // Get messages between the two users
      const messages = await MessageModel.find({
        $or: [
          { sender: currentUserId, recipient: friendId },
          { sender: friendId, recipient: currentUserId }
        ]
      })
      .populate('sender', 'username email')
      .sort({ createdAt: -1 })
      .limit(limit * 1)
      .skip((page - 1) * limit);

      // Mark received messages as read
      await MessageModel.updateMany(
        { sender: friendId, recipient: currentUserId, isRead: false },
        { isRead: true }
      );

      res.json({
        messages: messages.reverse(), // Return in chronological order
        page: parseInt(page),
        hasMore: messages.length === parseInt(limit)
      });
    } catch (error) {
      console.error('Get chat history error:', error);
      res.status(500).json({ message: 'Failed to get chat history' });
    }
  },

  // Send room invitation via chat
  async sendRoomInvitation(req, res) {
    try {
      const { friendId, roomId } = req.params;
      const currentUserId = req.user.userId;

      // Verify friendship
      const friendship = await FriendModel.findOne({
        $or: [
          { requester: currentUserId, recipient: friendId },
          { requester: friendId, recipient: currentUserId }
        ],
        status: 'accepted'
      });

      if (!friendship) {
        return res.status(403).json({ message: 'You can only invite friends' });
      }

      // Verify room access and get room details
      const room = await RoomModel.findOne({
        _id: roomId,
        'members.user': currentUserId,
        'members.role': { $in: ['owner', 'admin'] }
      });

      if (!room) {
        return res.status(403).json({ message: 'You can only invite to rooms you own or admin' });
      }

      // Generate invite code if needed
      if (!room.inviteCode) {
        room.inviteCode = Math.random().toString(36).substring(2, 15) + 
                         Math.random().toString(36).substring(2, 15);
        await room.save();
      }

      // Create invitation message
      const message = new MessageModel({
        sender: currentUserId,
        recipient: friendId,
        content: `🏠 You've been invited to join "${room.name}"! Click to join this debate room.`,
        messageType: 'room_invite',
        roomInvite: {
          roomId: room._id,
          roomName: room.name,
          inviteCode: room.inviteCode
        }
      });

      await message.save();
      await message.populate('sender', 'username email');

      res.status(201).json(message);
    } catch (error) {
      console.error('Send room invitation error:', error);
      res.status(500).json({ message: 'Failed to send room invitation' });
    }
  }
};
