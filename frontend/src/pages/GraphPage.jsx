// Status: in progress
/* Basically the MVP that I'm trying to make right now, shows graph, shows interactive buttons, works */

import React, { useState, useEffect } from 'react';
import { socket } from '../lib/socket';
import GraphView from '../components/GraphView';
import ToolBar from '../components/ToolBar';
import EntityForm from '../components/EntityForm';
import * as graphFunctions from '../functions/graphFunctions'; // imports all the functions

export default function GraphPage() {

  // window dimensions
  const [dimensions, setDimensions] = useState({
    width: window.innerWidth,
    height: window.innerHeight
  });

  // init variables
  const [data, setData] = useState({ nodes: [], links: [] });
  const [selection, setSelection] = useState({ type: null, item: null });
  const [form, setForm] = useState(null);

  // Fetch initial graph from backend
  useEffect(() => {
    async function loadGraph() {
      try {
        const res = await fetch('http://localhost:3000/api/graph');
        const json = await res.json(); // parse into json format
        setData(graphFunctions.transformGraph(json));
      } catch (err) {
        console.error('Error loading graph:', err);
      }
    }
    loadGraph();
  }, []);

  // Listen for real-time graph updates
  useEffect(() => {
    const handleUpdate = (json) => {
      setData(graphFunctions.transformGraph(json));
    };

    socket.on('graph-updated', handleUpdate);
    return () => socket.off('graph-updated', handleUpdate);
  }, []); 

  // Handle window resize
  useEffect(() => {
    const onResize = () => setDimensions({
      width: window.innerWidth,
      height: window.innerHeight
    });
    window.addEventListener('resize', onResize);
    return () => window.removeEventListener('resize', onResize);
  }, []);

  const handleAddNode = () => setForm({ type: 'node', parent: null });
  const handleAddConnectedNode = node => setForm({ type: 'node', parent: node.id });
  const handleChallenge = item => {
    const type = selection.type === 'node' ? 'challenge-node' : 'challenge-edge';
    setForm({ type, item });
  };

  const handleDelete = item => {
    if (selection.type === 'node') {
      graphFunctions.deleteNode(item.id);
    } else if (selection.type === 'edge') {
      const src = item.source.id || item.source;
      const tgt = item.target.id || item.target;
      graphFunctions.deleteEdge(src,tgt);
    }
    setSelection({ type: null, item: null });
  };

  const handleClear = () => setSelection({ type: null, item: null });

  const handleSubmitForm = formData => {
    if (form.type === 'node') {
      const newNode = {
        id: Date.now().toString(),
        label: formData.label,
        metadata: {},
        x: Math.random() * 400,
        y: Math.random() * 400
      };
      graphFunctions.addNode(newNode.label);
      if (form.parent) {
        socket.emit('add-edge', {
          source: form.parent,
          target: newNode.id,
          metadata: {}
        });
      }
    } else if (form.type === 'challenge-node') {
      graphFunctions.challengeNode(id, challenger, responder,formData.reason)
    } else if (form.type === 'challenge-edge') {
      const src = form.item.source.id || form.item.source;
      const tgt = form.item.target.id || form.item.target;
      graphFunctions.addChallenge("edge", id, challenger, responder,formData.reason)
      socket.emit('challenge-edge', {
        source: src,
        target: tgt,
        reason: formData.reason
      });
    }

    setForm(null);
    setSelection({ type: null, item: null });
  };

  return (
    <>
      <GraphView
        graphData={data}
        dimensions={dimensions}
        selection={selection}
        setSelection={setSelection}
      />
      <ToolBar
        selection={selection}
        onAddNode={handleAddNode}
        onAddConnectedNode={handleAddConnectedNode}
        onChallenge={handleChallenge}
        onDelete={handleDelete}
        onClearSelection={handleClear}
      />
      {form && (
        <EntityForm form={form} onSubmit={handleSubmitForm} onCancel={() => setForm(null)} />
      )}
    </>
  );
}
