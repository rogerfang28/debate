// * Status: Good
// * Directs path to the correct page
import React from 'react';
import { BrowserRouter as Router, Routes, Route } from 'react-router-dom';
import HomePage from './pages/HomePage/HomePage';
import GraphPage from './pages/GraphPage/GraphPage';
import ProfilePage from './pages/ProfilePage/ProfilePage';
import PublicDebatesPage from './pages/PublicDebatesPage/PublicDebatesPage';
import PublicDebateViewerPage from './pages/PublicDebateViewerPage/PublicDebateViewerPage';
import Renderer from "./renderers/Renderer.jsx";

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
        <Route path="/test" element={<Renderer />} />
      </Routes>
    </Router>
  );
}
// import React from "react";
// import { BrowserRouter, Routes, Route } from "react-router-dom";
// import Renderer from "./renderers/Renderer.jsx";

// export default function App() {
//   return (
//     <BrowserRouter>
//       <Routes>
//         <Route path="/" element={<Renderer />} />
//       </Routes>
//     </BrowserRouter>
//   );
// }
