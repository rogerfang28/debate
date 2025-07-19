// src/pages/PublicDebatesPage/PublicDebatesPage.jsx
import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { API_BASE } from '../../lib/config';
import './PublicDebatesPage.css';

export default function PublicDebatesPage() {
  const [rooms, setRooms] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);
  const [searchQuery, setSearchQuery] = useState('');
  const [page, setPage] = useState(1);
  const [pagination, setPagination] = useState(null);
  const navigate = useNavigate();

  useEffect(() => {
    fetchPublicRooms();
  }, [page, searchQuery]);

  const fetchPublicRooms = async () => {
    setLoading(true);
    try {
      const queryParams = new URLSearchParams({
        page: page.toString(),
        limit: '12',
        search: searchQuery
      });

      const response = await fetch(`${API_BASE}/api/rooms/public?${queryParams}`);
      if (response.ok) {
        const data = await response.json();
        setRooms(data.rooms);
        setPagination(data.pagination);
        setError(null);
      } else {
        setError('Failed to fetch public debates');
      }
    } catch (err) {
      console.error('Error fetching public rooms:', err);
      setError('Failed to connect to server');
    } finally {
      setLoading(false);
    }
  };

  const handleSearch = (e) => {
    e.preventDefault();
    setPage(1); // Reset to first page when searching
    fetchPublicRooms();
  };

  const viewDebate = (roomId) => {
    navigate(`/public-debate/${roomId}`);
  };

  const goHome = () => {
    navigate('/');
  };

  const formatDate = (dateString) => {
    return new Date(dateString).toLocaleDateString('en-US', {
      year: 'numeric',
      month: 'short',
      day: 'numeric'
    });
  };

  return (
    <div className="public-debates-page">
      <div className="page-header">
        <button onClick={goHome} className="back-button">
          ← Back to Home
        </button>
        <h1>🌍 Public Debates</h1>
        <p className="page-description">
          Explore public debates and see how different perspectives connect
        </p>
      </div>

      <div className="search-section">
        <form onSubmit={handleSearch} className="search-form">
          <input
            type="text"
            placeholder="Search debates by name or topic..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            className="search-input"
          />
          <button type="submit" className="search-button">
            🔍 Search
          </button>
        </form>
      </div>

      {loading ? (
        <div className="loading-state">
          <div className="loading-spinner"></div>
          <p>Loading public debates...</p>
        </div>
      ) : error ? (
        <div className="error-state">
          <p className="error-message">❌ {error}</p>
          <button onClick={fetchPublicRooms} className="retry-button">
            Try Again
          </button>
        </div>
      ) : (
        <>
          <div className="debates-grid">
            {rooms.length === 0 ? (
              <div className="empty-state">
                <h3>No public debates found</h3>
                <p>
                  {searchQuery 
                    ? `No debates match "${searchQuery}". Try a different search term.`
                    : 'No public debates are available at the moment.'
                  }
                </p>
              </div>
            ) : (
              rooms.map((room) => (
                <div key={room._id} className="debate-card">
                  <div className="debate-header">
                    <h3 className="debate-title">{room.name}</h3>
                    <span className="member-count">
                      👥 {room.memberCount} member{room.memberCount !== 1 ? 's' : ''}
                    </span>
                  </div>
                  
                  {room.description && (
                    <p className="debate-description">{room.description}</p>
                  )}
                  
                  <div className="debate-meta">
                    <span className="debate-owner">
                      Created by: {room.owner?.username || room.owner?.email}
                    </span>
                    <span className="debate-date">
                      {formatDate(room.createdAt)}
                    </span>
                  </div>
                  
                  <button 
                    onClick={() => viewDebate(room._id)}
                    className="view-button"
                  >
                    👁️ View Debate
                  </button>
                </div>
              ))
            )}
          </div>

          {pagination && pagination.pages > 1 && (
            <div className="pagination">
              <button
                onClick={() => setPage(page - 1)}
                disabled={page <= 1}
                className="pagination-button"
              >
                ← Previous
              </button>
              
              <span className="pagination-info">
                Page {pagination.page} of {pagination.pages}
              </span>
              
              <button
                onClick={() => setPage(page + 1)}
                disabled={page >= pagination.pages}
                className="pagination-button"
              >
                Next →
              </button>
            </div>
          )}
        </>
      )}
    </div>
  );
}
