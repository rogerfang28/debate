import { useState } from 'react';
import { reconnectSocket } from '../../lib/socket';
import { API_BASE } from '../../lib/config';
import AuthInput from './AuthInput';
import AuthErrorBox from './AuthErrorBox';

export default function SignupForm({ onAuthSuccess }) {
  const [email, setEmail] = useState('');
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [error, setError] = useState('');
  const [isLoading, setIsLoading] = useState(false);

  const handleSignup = async (e) => {
    e.preventDefault();
    setError('');
    setIsLoading(true);

    try {
      const res = await fetch(`${API_BASE}/auth/signup`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ email, username, password })
      });

      const json = await res.json();
      if (res.ok) {
        localStorage.setItem('token', json.token);
        reconnectSocket();
        onAuthSuccess(json.token);
      } else {
        setError(json.message || 'Signup failed');
      }
    } catch {
      setError('Network error');
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <form onSubmit={handleSignup} className="space-y-4">
      <h2 className="text-xl font-semibold mb-2">Create Account</h2>
      {error && <AuthErrorBox message={error} />}
      <AuthInput
        id="email"
        label="Email"
        value={email}
        setValue={setEmail}
        placeholder="Enter your email"
        disabled={isLoading}
      />
      <AuthInput
        id="username"
        label="Username"
        value={username}
        setValue={setUsername}
        placeholder="Choose a username"
        disabled={isLoading}
      />
      <AuthInput
        id="password"
        label="Password"
        type="password"
        value={password}
        setValue={setPassword}
        placeholder="Create a password"
        disabled={isLoading}
      />
      <button type="submit" disabled={isLoading} className="w-full">
        {isLoading ? 'Creating account...' : '🚀 Create Account'}
      </button>
    </form>
  );
}
