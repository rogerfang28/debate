import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  server: {
    proxy: {
      '/api': 'http://localhost:3000'  // Proxy all /api requests to backend
    },
    historyApiFallback: true  // Enable SPA fallback for development
  },
  build: {
    rollupOptions: {
      // Ensure proper SPA routing in production build
      output: {
        manualChunks: undefined
      }
    }
  }
})
