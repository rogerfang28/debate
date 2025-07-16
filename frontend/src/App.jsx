// Status: Good I think, not much can go wrong here, it's kind of tool 1 and 2 I guess, decides where stuff is and renders
// src/App.jsx
import React from 'react';
import GraphPage from './pages/GraphPage';

export default function App() {
  return (
    <div style={{ height: '100vh', display: 'flex', flexDirection: 'column' }}>
      <h1 style={{zIndex:50}}>
        My Graph App
      </h1>

      <main>
        <GraphPage />
      </main>
    </div>
  );
}
