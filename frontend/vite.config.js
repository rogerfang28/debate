import { defineConfig, loadEnv } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), '')
  const apiUrl = env.VITE_API_URL?.trim()

  if (!apiUrl) {
    throw new Error('VITE_API_URL must be set')
  }

  return {
    plugins: [react()],
    server: {
      host: "0.0.0.0",
      port: 3000,
      strictPort: true,
      proxy: {
        '/api': {
          target: apiUrl,
          changeOrigin: true,
          rewrite: (path) => path.replace(/^\/api/, ''),
        },
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
  }
})
