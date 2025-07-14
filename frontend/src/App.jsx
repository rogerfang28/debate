import React, { useState, useEffect, useRef } from 'react';
import ForceGraph2D from 'react-force-graph-2d';
import './App.css'

export default function App() {
  const fgRef = useRef();

  const [dimensions, setDimensions] = useState({
    width: window.innerWidth,
    height: window.innerHeight
  });

  useEffect(() => {
    const onResize = () => setDimensions({
      width: window.innerWidth,
      height: window.innerHeight
    });
    window.addEventListener('resize', onResize);
    return () => window.removeEventListener('resize', onResize);
  }, []);

  const [data] = useState({
    nodes: [
      { id: '1', text: 'A' },
      { id: '2', text: 'B' }
    ],
    links: [{ source: '1', target: '2' }]
  });

  return (
    <>
      <h1>Graph</h1>
      <div style={{ width: dimensions.width, height: dimensions.height, position: 'fixed', top: 0, left: 0 }}>
        <ForceGraph2D
          ref={fgRef}
          graphData={data}
          nodeLabel="text"
          nodeAutoColorBy="id"
          width={dimensions.width}
          height={dimensions.height}
        />
      </div>
    </>
  );
}
