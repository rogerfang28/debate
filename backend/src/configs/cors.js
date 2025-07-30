import cors from 'cors';
import { Server } from 'socket.io'

export default function setupCorsAndSocket(app,server){
    const allowedOrigins = [
        'http://localhost:5173',
        'https://debate-frontend.onrender.com'
    ];

    const io = new Server(server, { 
        cors: { 
            origin: allowedOrigins,
            methods: ["GET", "POST"],
            credentials: true
        } 
    });

    app.use(
        cors({
            origin: function (origin, callback) {
            if (!origin || allowedOrigins.includes(origin)) {
                callback(null, true);
            } else {
                callback(new Error('Not allowed by CORS'));
            }
            },
            credentials: true
        })
    );
}