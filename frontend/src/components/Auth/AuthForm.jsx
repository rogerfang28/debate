import { useState } from 'react';
import LoginForm from './LoginForm';
import SignupForm from './SignupForm';

export default function AuthForm({ onAuthSuccess }) {
  const [mode, setMode] = useState('login'); // 'login' or 'signup'

  const toggleMode = () => {
    setMode((prev) => (prev === 'login' ? 'signup' : 'login'));
  };

  return (
    <div className="animate-fade-in max-w-md mx-auto">
      <div className="text-center mb-6">
        <h2 className="text-2xl font-semibold mb-2">
          {mode === 'login' ? 'Welcome Back' : 'Join DebateGraph'}
        </h2>
        <p className="text-gray-400">
          {mode === 'login'
            ? 'Sign in to continue your debates'
            : 'Create your account to start debating'}
        </p>
      </div>

      {mode === 'login' ? (
        <LoginForm onAuthSuccess={onAuthSuccess} />
      ) : (
        <SignupForm onAuthSuccess={onAuthSuccess} />
      )}

      <div className="mt-6 text-center">
        <p className="text-sm text-gray-400 mb-3">
          {mode === 'login'
            ? "Don't have an account?"
            : 'Already have an account?'}
        </p>
        <button
          onClick={toggleMode}
          className="btn-secondary w-full"
        >
          {mode === 'login' ? '📝 Create New Account' : '🔑 Sign In Instead'}
        </button>
      </div>
    </div>
  );
}
