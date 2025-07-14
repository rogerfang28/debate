import React, { useState, useEffect, useRef } from 'react';
import ForceGraph2D from 'react-force-graph-2d';
import './App.css';
import { socket } from './socket';
import GraphView from './components/GraphView';
import ToolBar from './components/ToolBar';

export default function App() {
  const fgRef = useRef();

  const [dimensions, setDimensions] = useState({
    width: window.innerWidth,
    height: window.innerHeight
  });

  useEffect(() => {
    socket.on('graph-updated', newGraph => setData(newGraph));
    return () => socket.off('graph-updated');
  }, []);

  useEffect(() => {
    const onResize = () => setDimensions({
      width: window.innerWidth,
      height: window.innerHeight
    });
    window.addEventListener('resize', onResize);
    return () => window.removeEventListener('resize', onResize);
  }, []);

  const [data, setData] = useState({
    nodes: [
      { id: '1', text: 'A' },
      { id: '2', text: 'B' }
    ],
    links: [{ source: '1', target: '2' }]
  });

  return (
    <>
      <h1>Graph</h1>
      <GraphView graphData={data} dimensions={dimensions} />
    </>
  );
}
