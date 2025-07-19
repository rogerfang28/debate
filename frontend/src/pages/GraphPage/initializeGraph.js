import { useEffect } from 'react';
import { socket } from '../../lib/socket';
import * as graph from '../../functions/graphFunctions';
import { API_BASE } from '../../lib/config';

export function useInitializeGraph(setData, roomId, draggedNodeId) {
  useEffect(() => {
    // Don't load graph if no roomId is provided
    if (!roomId) {
      console.log('No room ID provided, skipping graph initialization');
      return;
    }

    async function loadGraph() {
      try {
        const token = localStorage.getItem('token');
        console.log("Fetching room graph with token:", token ? 'Token present' : 'No token');
        
        if (!token) {
          console.error('No authentication token found');
          return;
        }

        // Fetch room-specific graph data
        const res = await fetch(`${API_BASE}/rooms/${roomId}/graph`, {
          headers: {
            'Authorization': `Bearer ${token}`,
            'Content-Type': 'application/json'
          }
        });

        if (!res.ok) {
          if (res.status === 401 || res.status === 403) {
            console.error('Authentication failed or access denied');
            localStorage.removeItem('token');
            window.location.href = '/';
            return;
          }
          throw new Error(`HTTP ${res.status}: ${res.statusText}`);
        }

        const json = await res.json();
        console.log('Loaded room graph data:', json);
        setData(graph.transformGraph(json));
      } catch (err) {
        console.error('Error loading room graph:', err);
        // If it's an auth error, redirect to login
        if (err.message.includes('401') || err.message.includes('403')) {
          localStorage.removeItem('token');
          window.location.href = '/';
        }
      }
    }
    
    loadGraph();

    // Listen for room-specific graph updates
    const update = json => {
      console.log('Received room graph update:', json);
      setData(prevData => {
        const newData = graph.transformGraph(json);
        
        // If there's a node being dragged, preserve its position to prevent conflicts
        if (prevData && prevData.nodes && draggedNodeId) {
          newData.nodes = newData.nodes.map(node => {
            if (node.id === draggedNodeId) {
              const prevNode = prevData.nodes.find(p => p.id === node.id);
              if (prevNode) {
                // Keep the dragged node at its current position
                return { ...node, x: prevNode.x, y: prevNode.y };
              }
            }
            return node;
          });
        }
        
        return newData;
      });
    };
    
    socket.on('room-graph-updated', update);
    return () => socket.off('room-graph-updated', update);
  }, [setData, roomId]);
}
