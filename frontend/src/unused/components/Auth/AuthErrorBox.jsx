export default function AuthErrorBox({ message }) {
  return (
    <div className="p-3 bg-red-900/50 border border-red-600/50 rounded-lg text-red-200 text-sm">
      {message}
    </div>
  );
}
