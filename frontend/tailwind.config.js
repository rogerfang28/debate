export default {
  content: {
    files: ["./src/**/*.{html,js,jsx,ts,tsx}"],
  },
  // Safelist all classes used in backend .pbtxt files
  safelist: [
    // Backgrounds
    'bg-gray-50', 'bg-gray-100', 'bg-gray-200', 'bg-gray-300', 'bg-gray-700', 'bg-gray-800', 'bg-gray-900',
    'bg-white',
    'bg-red-600', 'hover:bg-red-700',
    'bg-blue-600', 'hover:bg-blue-700',
    'bg-yellow-200', 'bg-yellow-600', 'hover:bg-yellow-700',
    'bg-green-400', 'bg-green-600', 'hover:bg-green-700',
    
    // Text colors
    'text-white', 'text-black',
    'text-gray-500', 'text-gray-600', 'text-gray-700', 'text-gray-800',
    'text-white-600', 'text-white-700', 'text-white-800',
    
    // Sizing
    'w-full', 'w-64', 'w-80', 'w-96',
    'h-48', 'h-full',
    'min-h-screen', 'min-h-64',
    'flex-1',
    
    // Spacing
    'p-2', 'p-4', 'p-6',
    'px-4', 'px-6', 'py-2', 'py-4',
    'm-2', 'mx-8', 'mb-2', 'mb-3', 'mb-4', 'mb-6', 'mb-8', 'mt-16',
    'gap-1', 'gap-2', 'gap-3', 'gap-4', 'gap-8',
    
    // Layout
    'flex', 'flex-col',
    'grid', 'grid-cols-1', 'grid-cols-4',
    'md:grid-cols-2', 'lg:grid-cols-3',
    'justify-between', 'items-start', 'items-center',
    'text-center',
    
    // Borders
    'border', 'border-2',
    'border-gray-300', 'border-gray-700',
    'rounded',
    
    // Text
    'text-xs', 'text-sm', 'text-lg',
    'font-semibold',
    'leading-relaxed',
    
    // Effects
    'transition-colors',
  ],
}
