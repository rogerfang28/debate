// * Status: Good
// * Directs path to the correct page
import React from 'react';
import { BrowserRouter as Router, Routes, Route } from 'react-router-dom';
import Renderer from "./renderers/Renderer";

const App: React.FC = () => {
  return (
    <Router>
      <Routes>
        <Route path="/" element={<Renderer />} />
      </Routes>
    </Router>
  );
};

export default App;
