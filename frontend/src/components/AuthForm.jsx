import { useState } from 'react';
import { reconnectSocket } from '../lib/socket';

export default function AuthForm({ onAuthSuccess }) {
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [mode, setMode] = useState('login'); // or 'signup'
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState('');

  const handleSubmit = async (e) => {
    e.preventDefault();
    setIsLoading(true);
    setError('');

    try {
      const res = await fetch(`http://localhost:3000/api/auth/${mode}`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ email, password })
      });

      const json = await res.json();
      if (res.ok) {
        localStorage.setItem('token', json.token);
        reconnectSocket(); // Reconnect socket with new token
        onAuthSuccess(json.token);
      } else {
        setError(json.message || 'Authentication failed. Please try again.');
      }
    } catch (err) {
      setError('Network error. Please check your connection and try again.');
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <div className="animate-fade-in">
      <div className="text-center mb-6">
        <h2 className="text-2xl font-semibold mb-2">
          {mode === 'login' ? 'Welcome Back' : 'Join DebateGraph'}
        </h2>
        <p className="text-gray-400">
          {mode === 'login' 
            ? 'Sign in to continue your debates' 
            : 'Create your account to start debating'
          }
        </p>
      </div>

      {error && (
        <div className="mb-4 p-3 bg-red-900/50 border border-red-600/50 rounded-lg text-red-200 text-sm animate-slide-in">
          {error}
        </div>
      )}

      <form onSubmit={handleSubmit} className="space-y-4">
        <div>
          <label htmlFor="email" className="block text-sm font-medium text-gray-300 mb-2">
            Email Address
          </label>
          <input
            id="email"
            type="email"
            value={email}
            placeholder="Enter your email"
            onChange={(e) => setEmail(e.target.value)}
            required
            disabled={isLoading}
            className={`w-full transition-all ${isLoading ? 'opacity-50 cursor-not-allowed' : ''}`}
          />
        </div>

        <div>
          <label htmlFor="password" className="block text-sm font-medium text-gray-300 mb-2">
            Password
          </label>
          <input
            id="password"
            type="password"
            value={password}
            placeholder="Enter your password"
            onChange={(e) => setPassword(e.target.value)}
            required
            disabled={isLoading}
            className={`w-full transition-all ${isLoading ? 'opacity-50 cursor-not-allowed' : ''}`}
          />
        </div>

        <button 
          type="submit" 
          disabled={isLoading}
          className={`w-full ${isLoading ? 'loading' : ''}`}
        >
          {isLoading ? (
            <span className="flex items-center justify-center gap-2">
              <span className="loading-spinner"></span>
              {mode === 'login' ? 'Signing In...' : 'Creating Account...'}
            </span>
          ) : (
            mode === 'login' ? '🔐 Sign In' : '🚀 Create Account'
          )}
        </button>
      </form>

      <div className="mt-6 text-center">
        <p className="text-sm text-gray-400 mb-3">
          {mode === 'login' ? "Don't have an account?" : 'Already have an account?'}
        </p>
        <button 
          onClick={() => {
            setMode(mode === 'login' ? 'signup' : 'login');
            setError('');
          }}
          className="btn-secondary w-full"
          disabled={isLoading}
        >
          {mode === 'login' ? '📝 Create New Account' : '🔑 Sign In Instead'}
        </button>
      </div>
    </div>
  );
}
