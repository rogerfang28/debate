// * Status: Complete
// * Function: Allows CORS origins for the URLs
import cors from 'cors';

export const allowedOrigins = [
  'http://localhost:5173',
  'https://debate-frontend.onrender.com',
  'http://192.168.86.250:5173/'
];

export default function configureCors() {
  return cors({
    origin(origin, callback) {
      if (!origin || allowedOrigins.includes(origin)) {
        callback(null, true);
      } else {
        callback(new Error('Not allowed by CORS'));
      }
    },
    credentials: true
  });
}