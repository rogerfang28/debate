import mongoose from 'mongoose';
import dotenv from 'dotenv';

dotenv.config();

export const connectDB = async() => {
  try {
    const uri = process.env.MONGODB_URI;
    const conn = await mongoose.connect(uri);
    console.log('MongoDB connected:',conn.connection.host);
  } catch (err) {
    console.error('MongoDB connection error:', err);
  }
}
