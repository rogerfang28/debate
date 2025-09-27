// src/database/lib/db.ts
import { MongoClient } from "mongodb";

const uri = process.env.MONGODB_URI!;
export const client = new MongoClient(uri);

let isConnected = false;
export async function connectDB() {
  if (!isConnected) {
    await client.connect();
    isConnected = true;
    console.log("✅ Mongo connected");
  }
}
