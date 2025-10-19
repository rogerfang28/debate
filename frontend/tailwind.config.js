export default {
  content: {
    files: ["./src/**/*.{html,js,jsx,ts,tsx}"],
  },
  // Safelist specific classes that come from backend
  safelist: [
    'bg-red-600',
    'hover:bg-red-700',
    'bg-blue-600',
    'hover:bg-blue-700',
    'text-white',
    'text-black',
    'text-gray-600',
    'text-gray-700',
    'text-gray-800',
    'w-full',
    'px-4',
    'py-2',
    'rounded',
    'transition-colors',
  ],
}
