import React from "react";
import { BaseComponentProps } from "./TextComponent";

interface GraphNode {
  id: string;
  text: string;
  x?: number;
  y?: number;
  type?: string;
}

interface GraphEdge {
  id: string;
  source: string;
  target: string;
  type?: string;
}

interface GraphComponentProps extends BaseComponentProps {
  component: BaseComponentProps['component'] & {
    nodes?: GraphNode[];
    edges?: GraphEdge[];
    selectedNodeId?: string;
    selectedEdgeId?: string;
  };
}

const GraphComponent: React.FC<GraphComponentProps> = ({ component, className, style }) => {
  const { nodes = [], edges = [], selectedNodeId } = component;

  // Don't render the large SVG if there's no data
  if (nodes.length === 0 && edges.length === 0) {
    return (
      <div
        id={component.id}
        className={`node-graph ${className || ''}`}
        style={{
          ...style,
          background: 'var(--surface-card)',
          border: '1px solid var(--border-subtle)',
          borderRadius: 'var(--radius-lg)',
          padding: 'var(--space-lg)',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          minHeight: '80px',
        }}
      >
        <p style={{ color: 'var(--text-muted)', fontSize: '0.875rem' }}>No graph data</p>
      </div>
    );
  }

  // Color mapping for node types
  const getNodeColor = (type?: string, isSelected?: boolean) => {
    if (isSelected) return { fill: 'var(--accent-indigo)', stroke: 'var(--accent-indigo-hover)' };
    switch (type) {
      case 'claim': return { fill: '#1e293b', stroke: 'var(--accent-blue)' };
      case 'evidence': return { fill: '#1e293b', stroke: 'var(--accent-green)' };
      case 'challenge': return { fill: '#1e293b', stroke: 'var(--accent-red)' };
      case 'counter': return { fill: '#1e293b', stroke: 'var(--accent-orange)' };
      default: return { fill: '#1e293b', stroke: 'var(--border-strong)' };
    }
  };

  const getEdgeColor = (type?: string) => {
    switch (type) {
      case 'supports': return 'var(--accent-green)';
      case 'challenges': return 'var(--accent-red)';
      case 'counters': return 'var(--accent-orange)';
      default: return 'var(--border-strong)';
    }
  };

  // Calculate viewBox from actual node positions instead of fixed 800x600
  const padding = 60;
  const xs = nodes.map(n => n.x || 0);
  const ys = nodes.map(n => n.y || 0);
  const minX = Math.min(...xs, 0) - padding;
  const minY = Math.min(...ys, 0) - padding;
  const maxX = Math.max(...xs, 100) + padding;
  const maxY = Math.max(...ys, 100) + padding;
  const vw = Math.max(maxX - minX, 200);
  const vh = Math.max(maxY - minY, 150);

  return (
    <div
      id={component.id}
      className={`node-graph ${className || ''}`}
      style={{
        ...style,
        background: 'var(--surface-card)',
        border: '1px solid var(--border-subtle)',
        borderRadius: 'var(--radius-lg)',
        overflow: 'hidden',
        position: 'relative',
      }}
    >
      <svg width="100%" height="100%" viewBox={`${minX} ${minY} ${vw} ${vh}`} preserveAspectRatio="xMidYMid meet" style={{ display: 'block' }}>
        <defs>
          <filter id="glow">
            <feGaussianBlur stdDeviation="3" result="coloredBlur" />
            <feMerge>
              <feMergeNode in="coloredBlur" />
              <feMergeNode in="SourceGraphic" />
            </feMerge>
          </filter>
          <marker
            id="arrowhead"
            markerWidth="10"
            markerHeight="7"
            refX="9"
            refY="3.5"
            orient="auto"
          >
            <polygon points="0 0, 10 3.5, 0 7" fill="var(--border-strong)" />
          </marker>
        </defs>

        {/* Render edges */}
        {edges.map((edge) => {
          const sourceNode = nodes.find(n => n.id === edge.source);
          const targetNode = nodes.find(n => n.id === edge.target);
          if (!sourceNode || !targetNode) return null;

          const sx = sourceNode.x || 100;
          const sy = sourceNode.y || 100;
          const tx = targetNode.x || 200;
          const ty = targetNode.y || 200;
          const color = getEdgeColor(edge.type);

          return (
            <g key={edge.id}>
              <line
                x1={sx}
                y1={sy}
                x2={tx}
                y2={ty}
                stroke={color}
                strokeWidth="2"
                strokeOpacity="0.6"
                markerEnd="url(#arrowhead)"
                className="transition-all"
              />
            </g>
          );
        })}

        {/* Render nodes */}
        {nodes.map((node) => {
          const isSelected = node.id === selectedNodeId;
          const { fill, stroke } = getNodeColor(node.type, isSelected);
          const cx = node.x || Math.random() * 700 + 50;
          const cy = node.y || Math.random() * 500 + 50;
          const r = isSelected ? 34 : 28;

          return (
            <g key={node.id} className="cursor-pointer">
              {/* Glow ring for selected */}
              {isSelected && (
                <circle
                  cx={cx}
                  cy={cy}
                  r={r + 6}
                  fill="none"
                  stroke={stroke}
                  strokeWidth="2"
                  strokeOpacity="0.3"
                  filter="url(#glow)"
                />
              )}
              {/* Node circle */}
              <circle
                cx={cx}
                cy={cy}
                r={r}
                fill={fill}
                stroke={stroke}
                strokeWidth={isSelected ? 3 : 2}
                className="transition-all"
              />
              {/* Node label */}
              <text
                x={cx}
                y={cy + 4}
                textAnchor="middle"
                fill="var(--text-primary)"
                fontSize="11"
                fontWeight="500"
                className="pointer-events-none"
              >
                {node.text.length > 12 ? node.text.slice(0, 12) + '…' : node.text}
              </text>
            </g>
          );
        })}
      </svg>
    </div>
  );
};

export default GraphComponent;
