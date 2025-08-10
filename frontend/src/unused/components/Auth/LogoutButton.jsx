export default function LogoutButton({ onLogout }) {
  const handleLogout = () => {
    localStorage.removeItem('token');
    onLogout();
  };

  return (
    <button onClick={handleLogout} className="btn-secondary w-full">
      🔓 Logout
    </button>
  );
}
