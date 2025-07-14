import reactLogo from './assets/react.svg'
import viteLogo from '/vite.svg'
import './App.css'
import React, { useState } from 'react';
import ForceGraph2D from 'react-force-graph-2d';

export default function App() {
  const [data] = useState({
    nodes: [
      { id: '1', text: 'A' },
      { id: '2', text: 'B' }
    ],
    links: [{ source: '1', target: '2' }]
  });

  return (
    <div style={{ width: '100%', height: '600px' }}>
      <ForceGraph2D
        graphData={data}
        nodeLabel="text"
        nodeAutoColorBy="id"
        width={800}
        height={600}
      />
    </div>
  );
}
