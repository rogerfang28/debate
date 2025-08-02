import React from 'react';
import { JoinRoomForm } from '../../components/JoinRoomForm';
import { CreateRoomModal } from '../../components/CreateRoomModal';
import { ManageRoomModal } from '../../components/ManageRoomModal';
import { FriendsModal } from '../../components/FriendsModal';
import AuthForm from '../../components/Auth/AuthForm';

// Enhanced component registry that maps JSON component names to actual React components
export const COMPONENT_REGISTRY = {
  // Form Components
  JoinRoomForm: (props) => <JoinRoomForm {...props} />,
  AuthForm: (props) => <AuthForm {...props} />,
  
  // Modal Components  
  CreateRoomModal: (props) => <CreateRoomModal {...props} />,
  ManageRoomModal: (props) => <ManageRoomModal {...props} />,
  FriendsModal: (props) => <FriendsModal {...props} />,
  
  // Graph Components (placeholders for now - will be enhanced)
  GraphCanvas: (props) => (
    <div className="graph-canvas-placeholder bg-gray-800 rounded p-8 text-center border-2 border-dashed border-gray-600">
      <div className="text-4xl mb-4">🎯</div>
      <div className="text-lg font-semibold mb-2">Graph Canvas</div>
      <div className="text-sm text-gray-400">Interactive graph editing interface</div>
      <div className="text-xs text-gray-500 mt-2">Room: {props.roomId || 'N/A'}</div>
    </div>
  ),
  
  GraphViewer: (props) => (
    <div className="graph-viewer-placeholder bg-gray-800 rounded p-8 text-center border-2 border-dashed border-gray-600">
      <div className="text-4xl mb-4">👁️</div>
      <div className="text-lg font-semibold mb-2">Graph Viewer</div>
      <div className="text-sm text-gray-400">Read-only graph visualization</div>
      <div className="text-xs text-gray-500 mt-2">
        {props.readOnly ? 'Read-only mode' : 'Interactive mode'}
      </div>
    </div>
  ),

  // Utility Components
  LoadingSpinner: () => (
    <div className="flex items-center justify-center p-4">
      <div className="animate-spin rounded-full h-8 w-8 border-b-2 border-blue-500"></div>
    </div>
  ),

  ErrorMessage: ({ message }) => (
    <div className="bg-red-900 border border-red-700 rounded p-4 text-red-100">
      <div className="flex items-center">
        <span className="text-xl mr-2">⚠️</span>
        <span>{message || 'An error occurred'}</span>
      </div>
    </div>
  )
};

// Helper function to check if a component exists in the registry
export const hasComponent = (componentName) => {
  return componentName in COMPONENT_REGISTRY;
};

// Helper function to render a component with error boundaries
export const renderComponent = (componentName, props = {}, key) => {
  const Component = COMPONENT_REGISTRY[componentName];
  
  if (!Component) {
    return (
      <div
        key={key}
        className="component-placeholder bg-orange-900 border border-orange-700 rounded p-4 text-orange-100"
        style={{ fontStyle: "italic" }}
      >
        <div className="flex items-center">
          <span className="text-xl mr-2">🔧</span>
          <span>Component not found: {componentName}</span>
        </div>
      </div>
    );
  }

  try {
    return <Component key={key} {...props} />;
  } catch (error) {
    console.error(`Error rendering component ${componentName}:`, error);
    return (
      <div
        key={key}
        className="component-error bg-red-900 border border-red-700 rounded p-4 text-red-100"
      >
        <div className="flex items-center">
          <span className="text-xl mr-2">💥</span>
          <span>Error in component: {componentName}</span>
        </div>
      </div>
    );
  }
};
