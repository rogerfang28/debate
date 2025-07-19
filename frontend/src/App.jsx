// Status: Good I think, not much can go wrong here, it's kind of tool 1 and 2 I guess, decides where stuff is and renders
// src/App.jsx
import React from 'react';
import { BrowserRouter as Router, Routes, Route } from 'react-router-dom';
import HomePage from './pages/HomePage/HomePage';
import GraphPage from './pages/GraphPage/GraphPage';
import ProfilePage from './pages/ProfilePage/ProfilePage';
import PublicDebatesPage from './pages/PublicDebatesPage/PublicDebatesPage';
import PublicDebateViewerPage from './pages/PublicDebateViewerPage/PublicDebateViewerPage';

export default function App() {
  return (
    <Router>
      <Routes>
        <Route path="/" element={<HomePage />} />
        <Route path="/profile" element={<ProfilePage />} />
        <Route path="/graph/:roomId" element={<GraphPage />} />
        <Route path="/graph" element={<GraphPage />} /> {/* Legacy route - will redirect to home */}
        <Route path="/public-debates" element={<PublicDebatesPage />} />
        <Route path="/public-debate/:roomId" element={<PublicDebateViewerPage />} />
      </Routes>
    </Router>
  );
}
