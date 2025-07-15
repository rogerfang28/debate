import { io } from 'socket.io-client';
const URL = process.env.NODE_ENV === 'production'
  ? undefined
  : 'http://localhost:3000'; // backend URL during dev
export const socket = io(URL);
