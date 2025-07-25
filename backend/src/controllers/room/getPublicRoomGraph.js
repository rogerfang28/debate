import { RoomModel } from '../../models/Room.js';
import { NodeModel } from '../../models/Node.js';
import { EdgeModel } from '../../models/Edge.js';
import { ChallengeModel } from '../../models/Challenge.js';
import mongoose from 'mongoose';

/**
 * Get public room graph data (no authentication required)
 */
export async function getPublicRoomGraph(req, res) {
  try {
    const { roomId } = req.params;

    if (!mongoose.Types.ObjectId.isValid(roomId)) {
      return res.status(400).json({ message: 'Invalid room ID' });
    }

    const room = await RoomModel.findById(roomId);
    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    if (!room.isPublic) {
      return res.status(403).json({ message: 'This room is not public' });
    }

    const [nodes, edges, challenges] = await Promise.all([
      NodeModel.find({ room: roomId }),
      EdgeModel.find({ room: roomId }),
      ChallengeModel.find({ room: roomId, status: 'pending' })
    ]);

    res.json({
      room: {
        _id: room._id,
        name: room.name,
        description: room.description,
        createdAt: room.createdAt
      },
      nodes,
      edges,
      challenges
    });
  } catch (error) {
    console.error('Get public room graph error:', error);
    res.status(500).json({ message: 'Failed to fetch public room graph' });
  }
}
