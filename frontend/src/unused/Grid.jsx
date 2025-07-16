// Status: broken, unused but want to fix
// Grid.jsx should make a grid as the background that moves so that it shows how the nodes are moving
// Outputs: Grid
import React, { useRef, useEffect } from 'react';

export default function Grid({ width, height, step = 50, fgRef }) {
  const canvasRef = useRef();
  const animationRef = useRef();

  const draw = () => {
    const canvas = canvasRef.current;
    const ctx = canvas?.getContext('2d');
    if (!canvas || !ctx || !fgRef.current) return;

    const zoom = fgRef.current.zoom(); // current zoom level
    const camera = fgRef.current.camera(); // THREE.js camera

    const panX = -camera.position.x * zoom;
    const panY = -camera.position.y * zoom;

    ctx.clearRect(0, 0, width, height);
    ctx.save();

    ctx.translate(width / 2 + panX, height / 2 + panY); // center + pan
    ctx.scale(zoom, zoom); // zoom level

    ctx.strokeStyle = '#ddd';
    ctx.lineWidth = 1 / zoom;

    const offsetX = (panX / zoom) % step;
    const offsetY = (panY / zoom) % step;

    const cols = Math.ceil(width / (step * zoom));
    const rows = Math.ceil(height / (step * zoom));

    for (let i = -1; i < cols + 1; i++) {
      const x = i * step - offsetX;
      ctx.beginPath();
      ctx.moveTo(x, -height);
      ctx.lineTo(x, height);
      ctx.stroke();
    }

    for (let j = -1; j < rows + 1; j++) {
      const y = j * step - offsetY;
      ctx.beginPath();
      ctx.moveTo(-width, y);
      ctx.lineTo(width, y);
      ctx.stroke();
    }

    ctx.restore();
  };

  useEffect(() => {
    const loop = () => {
      draw();
      animationRef.current = requestAnimationFrame(loop);
    };

    animationRef.current = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(animationRef.current);
  }, [width, height, step]);

  return (
    <canvas
      ref={canvasRef}
      width={width}
      height={height}
      style={{
        position: 'absolute',
        top: 0,
        left: 0,
        zIndex: 0,
        pointerEvents: 'none'
      }}
    />
  );
}
