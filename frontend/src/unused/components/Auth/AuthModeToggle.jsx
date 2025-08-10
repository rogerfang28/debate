export default function AuthModeToggle({ mode, setMode, setError, isLoading }) {
  return (
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
  );
}
