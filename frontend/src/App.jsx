// Enhanced App with navigation header
import React from 'react';
import { BrowserRouter as Router, Routes, Route, useLocation, useNavigate } from 'react-router-dom';
import HomePage from './pages/HomePage/HomePage';
import GraphPage from './pages/GraphPage/GraphPage';

function Navigation() {
  const location = useLocation();
  const navigate = useNavigate();
  const isGraphPage = location.pathname === '/graph';
  const token = localStorage.getItem('token');

  const handleLogout = () => {
    localStorage.removeItem('token');
    navigate('/');
  };

  // Don't show navigation on homepage when not logged in
  if (location.pathname === '/' && !token) {
    return null;
  }

  return (
    <nav className="app-navigation">
      <div className="nav-content">
        <div className="nav-brand" onClick={() => navigate('/')}>
          <span className="brand-icon">🧠</span>
          <span className="brand-text">DebateGraph</span>
        </div>
        
        {token && (
          <div className="nav-actions">
            {!isGraphPage && (
              <button 
                onClick={() => navigate('/graph')} 
                className="nav-button btn-primary"
              >
                📊 Graph View
              </button>
            )}
            {isGraphPage && (
              <button 
                onClick={() => navigate('/')} 
                className="nav-button btn-secondary"
              >
                🏠 Home
              </button>
            )}
            <button 
              onClick={handleLogout} 
              className="nav-button btn-secondary"
            >
              👋 Logout
            </button>
          </div>
        )}
      </div>
    </nav>
  );
}

export default function App() {
  return (
    <Router>
      <div className="app-container">
        <Navigation />
        <main className="app-main">
          <Routes>
            <Route path="/" element={<HomePage />} />
            <Route path="/graph" element={<GraphPage />} />
          </Routes>
        </main>
      </div>
    </Router>
  );
}
