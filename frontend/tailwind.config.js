export default {
  content: {
    files: ["./src/**/*.{html,js,jsx,ts,tsx}"],
  },
  // Safelist all classes used in backend .pbtxt files
  safelist: [
    // Backgrounds
    'bg-gray-50', 'bg-gray-100', 'bg-gray-200', 'bg-gray-300', 'bg-gray-600', 'bg-gray-700', 'bg-gray-800', 'bg-gray-900',
    'bg-white', 'bg-transparent',
    'bg-red-600', 'hover:bg-red-700', 'bg-red-500', 'hover:bg-red-500',
    'bg-blue-600', 'hover:bg-blue-700', 'bg-blue-500', 'hover:bg-blue-500',
    'bg-yellow-200', 'bg-yellow-600', 'hover:bg-yellow-700',
    'bg-green-400', 'bg-green-600', 'hover:bg-green-700', 'bg-green-500', 'hover:bg-green-500',
    'bg-slate-600', 'bg-slate-700', 'bg-slate-800', 'hover:bg-slate-500', 'hover:bg-slate-600',
    
    // Gradient backgrounds
    'bg-gradient-to-r', 'bg-gradient-to-br',
    'from-blue-400', 'to-purple-400',
    'from-red-600', 'to-red-500',
    'from-green-600', 'to-green-500',
    'from-blue-600', 'to-blue-500',
    'from-slate-800', 'to-slate-700',
    'from-gray-900', 'to-gray-800',
    
    // Background with opacity
    'bg-blue-400/10', 'bg-red-500/10', 'bg-white/20', 'bg-gray-800/90', 'bg-black/50',
    
    // Text colors
    'text-white', 'text-black', 'text-transparent',
    'text-gray-300', 'text-gray-400', 'text-gray-500', 'text-gray-600', 'text-gray-700', 'text-gray-800',
    'text-blue-400', 'text-red-300',
    'text-slate-200', 'text-slate-300', 'text-slate-400',
    'text-white/80', 'hover:text-white',
    
    // Text clip
    'bg-clip-text',
    
    // Sizing
    'w-full', 'w-20', 'w-64', 'w-80', 'w-96',
    'h-20', 'h-48', 'h-full',
    'min-h-screen', 'min-h-64',
    'min-w-64',
    'max-w-sm', 'max-w-md', 'max-w-2xl', 'max-w-4xl', 'max-w-7xl',
    'flex-1',
    
    // Spacing
    'p-1', 'p-2', 'p-3', 'p-4', 'p-6', 'p-8',
    'px-3', 'px-4', 'px-6', 'py-1.5', 'py-2', 'py-3', 'py-4',
    'pt-3', 'pb-3',
    'm-2', 'mx-auto', 'mx-4', 'mx-8', 'mb-2', 'mb-3', 'mb-4', 'mb-6', 'mb-8', 'mt-4', 'mt-8', 'mt-16',
    'gap-1', 'gap-2', 'gap-3', 'gap-4', 'gap-6', 'gap-8',
    'space-y-4',
    
    // Layout
    'flex', 'flex-col',
    'grid', 'grid-cols-1', 'grid-cols-2', 'grid-cols-4',
    'md:grid-cols-2', 'lg:grid-cols-3', 'xl:grid-cols-4',
    'justify-between', 'justify-center', 'items-start', 'items-center',
    'text-center',
    'overflow-hidden',
    'hidden',
    
    // Positioning
    'absolute', 'relative', 'fixed',
    'top-4', 'bottom-4', 'left-4', 'right-4',
    'inset-0',
    'z-50',
    
    // Borders
    'border', 'border-2', 'border-b',
    'border-gray-300', 'border-gray-600', 'border-gray-700', 'hover:border-gray-600',
    'border-slate-600',
    'border-white/10', 'border-red-500/30',
    'rounded', 'rounded-lg', 'rounded-md', 'rounded-2xl', 'rounded-full',
    'border-none',
    
    // Text
    'text-xs', 'text-sm', 'text-lg', 'text-xl', 'text-2xl', 'text-3xl', 'text-4xl', 'text-6xl',
    'font-bold', 'font-semibold', 'font-medium',
    'leading-relaxed',
    'placeholder-slate-400', 'placeholder-gray-400',
    
    // Effects
    'transition-colors', 'transition-all', 'transition',
    'shadow-2xl', 'shadow',
    'backdrop-blur-sm',
    
    // Hover/Focus/Disabled states
    'hover:transform', 'hover:-translate-y-0.5', 'hover:scale-105',
    'focus:ring-2', 'focus:ring-blue-500', 'focus:border-transparent',
    'disabled:opacity-50',
    'cursor-pointer',
    
    // Resize
    'resize-none',
    
    // Responsive
    'sm:px-6', 'lg:px-8',
  ],
}
