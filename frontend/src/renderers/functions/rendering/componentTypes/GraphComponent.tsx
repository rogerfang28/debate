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

const GraphComponent: React.FC<GraphComponentProps> = ({ component, className, style, events }) => {
  const { nodes = [], edges = [] } = component;

  return (
    <div className={className} style={style} {...events}>
      <svg width="100%" height="100%" viewBox="0 0 800 600">
        {/* Render edges */}
        {edges.map((edge) => {
          const sourceNode = nodes.find(n => n.id === edge.source);
          const targetNode = nodes.find(n => n.id === edge.target);
          
          if (!sourceNode || !targetNode) return null;
          
          return (
            <line
              key={edge.id}
              x1={sourceNode.x || 100}
              y1={sourceNode.y || 100}
              x2={targetNode.x || 200}
              y2={targetNode.y || 200}
              stroke="#4b5563"
              strokeWidth="2"
              className="hover:stroke-blue-400 cursor-pointer"
            />
          );
        })}
        
        {/* Render nodes */}
        {nodes.map((node) => (
          <g key={node.id}>
            <circle
              cx={node.x || Math.random() * 700 + 50}
              cy={node.y || Math.random() * 500 + 50}
              r="30"
              fill="#1f2937"
              stroke="#4b5563"
              strokeWidth="2"
              className="hover:fill-blue-600 cursor-pointer transition-colors"
            />
            <text
              x={node.x || Math.random() * 700 + 50}
              y={node.y || Math.random() * 500 + 55}
              textAnchor="middle"
              fill="white"
              fontSize="12"
              className="pointer-events-none"
            >
              {node.text.slice(0, 10)}...
            </text>
          </g>
        ))}
      </svg>
    </div>
  );
};

export default GraphComponent;
