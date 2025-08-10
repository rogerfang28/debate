import { RoomModel } from '../../../../../server/models/Room.js';
import mongoose from 'mongoose';

/**
 * Get all rooms where the user is either owner or member
 * @param userId - The ID of the user to get rooms for
 * @returns Promise<Array> - Array of room objects
 * @throws Error if database query fails
 * @author GitHub Copilot
 */
export default async function getRooms(userId: string) {
  try {
    // Find rooms where user is either owner or member
    const testId = new mongoose.Types.ObjectId("687ac6a3b292e5fa181ed2f1");
    console.log("🔍 Fetching rooms for user ID:", testId.toString());
    const rooms = await RoomModel.find({
      $or: [
        { owner: testId }, // User is the owner
        { 'members.user': testId } // User is a member
      ]
    })
    .populate('owner', 'username email') // Populate owner details
    .populate('members.user', 'username email') // Populate member details
    .sort({ updatedAt: -1 }) // Sort by most recently updated
    .lean(); // Return plain JavaScript objects for better performance

    return rooms;
  } catch (error) {
    console.error('Error fetching user rooms:', error);
    throw new Error('Failed to fetch user rooms');
  }
}