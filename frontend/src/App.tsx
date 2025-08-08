// * Status: Good
// * Directs path to the correct page
import React from 'react';
import { BrowserRouter as Router, Routes, Route } from 'react-router-dom';
import HomePage from './pages/HomePage/HomePage';
import GraphPage from './pages/GraphPage/GraphPage';
import ProfilePage from './pages/ProfilePage/ProfilePage';
import PublicDebatesPage from './pages/PublicDebatesPage/PublicDebatesPage';
import PublicDebateViewerPage from './pages/PublicDebateViewerPage/PublicDebateViewerPage';
// @ts-ignore - Renderer.jsx not converted yet
import Renderer from "./renderers/Renderer.jsx";

const App: React.FC = () => {
  return (
    <Router>
      <Routes>
        <Route path="/" element={<Renderer />} />
        <Route path="/profile" element={<ProfilePage />} />
        <Route path="/graph/:roomId" element={<GraphPage />} />
        <Route path="/graph" element={<GraphPage />} /> {/* Legacy route - will redirect to home */}
        <Route path="/public-debates" element={<PublicDebatesPage />} />
        <Route path="/public-debate/:roomId" element={<PublicDebateViewerPage />} />
        <Route path="/home" element={<HomePage />} />
      </Routes>
    </Router>
  );
};

export default App;
