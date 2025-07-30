import cors from 'cors';

/**
 * List of allowed origins for CORS.
 * @type {string[]}
 */
export const allowedOrigins = [
  'http://localhost:5173',
  'https://debate-frontend.onrender.com'
];

/**
 * Returns the configured CORS middleware.
 * @returns {import('express').RequestHandler}
 */
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