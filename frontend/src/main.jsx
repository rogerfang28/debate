// * Status: Complete
// * Entry point, nothing else about this file
import React from 'react';
import ReactDOM from 'react-dom/client';
import App from './App';
import './styles/index.css'

const container = document.getElementById('root');
const root = ReactDOM.createRoot(container);  
root.render(
  <React.StrictMode>
    <App />
  </React.StrictMode>
);
