// src/App.jsx
import React from 'react';
import GraphPage from './pages/GraphPage';

export default function App() {
  return (
    <div style={{ height: '100vh', display: 'flex', flexDirection: 'column' }}>
      <h1 style={{zIndex:50}}>
        My Graph App
      </h1>

      <main style={{ flex: 1, backgroundColor: '#2e2e2eff', position: 'relative' }}>
        <GraphPage />
      </main>
    </div>
  );
}
