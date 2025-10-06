import { useState } from 'react';
import { reconnectSocket } from '../../lib/socket';
import { API_BASE } from '../../lib/config';
import AuthInput from './AuthInput';
import AuthErrorBox from './AuthErrorBox';

export default function LoginForm({ onAuthSuccess }) {
  const [identifier, setIdentifier] = useState('');
  const [password, setPassword] = useState('');
  const [error, setError] = useState('');
  const [isLoading, setIsLoading] = useState(false);

  const handleLogin = async (e) => {
    e.preventDefault();
    setError('');
    setIsLoading(true);

    try {
      const res = await fetch(`${API_BASE}/auth/login`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ identifier, password })
      });

      const json = await res.json();
      if (res.ok) {
        localStorage.setItem('token', json.token);
        reconnectSocket();
        onAuthSuccess(json.token);
      } else {
        setError(json.message || 'Login failed');
      }
    } catch {
      setError('Network error');
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <form onSubmit={handleLogin} className="space-y-4">
      <h2 className="text-xl font-semibold mb-2">Sign In</h2>
      {error && <AuthErrorBox message={error} />}
      <AuthInput
        id="identifier"
        label="Email or Username"
        value={identifier}
        setValue={setIdentifier}
        placeholder="Enter email or username"
        disabled={isLoading}
      />
      <AuthInput
        id="password"
        label="Password"
        type="password"
        value={password}
        setValue={setPassword}
        placeholder="Enter your password"
        disabled={isLoading}
      />
      <button type="submit" disabled={isLoading} className="w-full">
        {isLoading ? 'Signing in...' : '🔐 Sign In'}
      </button>
    </form>
  );
}
