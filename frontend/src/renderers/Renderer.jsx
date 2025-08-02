import React, { useEffect, useState } from "react";
import { useLocation, useParams } from "react-router-dom";
import getInfo from "./functions/getInfo.js"; // make sure path is correct
import renderPage from "./functions/renderPage.jsx";

export default function Renderer() {
  const [page, setPage] = useState(null);
  const [isLoading, setIsLoading] = useState(true);
  const [error, setError] = useState(null);
  const location = useLocation();
  const params = useParams();

  // Determine which page to load based on current route
  const getPageName = () => {
    const pathname = location.pathname;
    
    if (pathname === '/' || pathname === '/home') {
      return 'home';
    } else if (pathname === '/profile') {
      return 'profile';
    } else if (pathname.startsWith('/graph/')) {
      return 'graph';
    } else if (pathname === '/public-debates') {
      return 'publicDebates';
    } else if (pathname.startsWith('/public-debate/')) {
      return 'publicDebateViewer';
    }
    
    return 'home'; // Default fallback
  };

  // Build query parameters for page-specific data
  const getQueryParams = () => {
    const searchParams = new URLSearchParams();
    
    // Add route parameters
    if (params.roomId) {
      searchParams.append('roomId', params.roomId);
    }
    
    // Add URL search parameters
    const urlParams = new URLSearchParams(location.search);
    for (const [key, value] of urlParams) {
      searchParams.append(key, value);
    }
    
    return searchParams.toString();
  };

  useEffect(() => {
    async function fetchData() {
      setIsLoading(true);
      setError(null);
      
      try {
        const pageName = getPageName();
        const queryParams = getQueryParams();
        const endpoint = `/api/data/${pageName}${queryParams ? `?${queryParams}` : ''}`;
        
        console.log(`🔄 Loading page: ${pageName} from ${endpoint}`);
        
        // Get JSON UI from backend via getInfo helper
        const data = await getInfo(endpoint);

        if (!data) {
          console.error("Renderer: No data returned from getInfo");
          setError("Failed to load page data");
          return;
        }

        console.log(`✅ Page loaded: ${pageName}`, data);
        setPage(data);
      } catch (err) {
        console.error("Renderer: Failed to load page", err);
        setError(err.message || "Failed to load page");
      } finally {
        setIsLoading(false);
      }
    }
    
    fetchData();
  }, [location.pathname, location.search, params.roomId]); // Re-fetch when route changes

  // Show loading state
  if (isLoading) {
    return (
      <div className="renderer-loading flex items-center justify-center min-h-screen bg-gray-900 text-white">
        <div className="text-center">
          <div className="text-4xl mb-4">🔄</div>
          <div className="text-lg">Loading page...</div>
        </div>
      </div>
    );
  }

  // Show error state
  if (error) {
    return (
      <div className="renderer-error flex items-center justify-center min-h-screen bg-gray-900 text-white">
        <div className="text-center max-w-md">
          <div className="text-4xl mb-4 text-red-400">⚠️</div>
          <div className="text-lg mb-2">Failed to load page</div>
          <div className="text-sm text-gray-400 mb-4">{error}</div>
          <button 
            onClick={() => window.location.reload()}
            className="bg-blue-600 hover:bg-blue-700 px-4 py-2 rounded text-sm"
          >
            🔄 Retry
          </button>
        </div>
      </div>
    );
  }

  // Render JSON-driven UI
  return page ? renderPage(page) : null;
}
