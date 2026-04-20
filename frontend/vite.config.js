import { defineConfig, loadEnv } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), '')
  const apiUrl = env.VITE_API_URL?.trim()
  const devHost = env.VITE_DEV_HOST?.trim()
  const devPortRaw = env.VITE_DEV_PORT?.trim()

  if (!apiUrl) {
    throw new Error('VITE_API_URL must be set')
  }
  if (!devHost) {
    throw new Error('VITE_DEV_HOST must be set')
  }
  if (!devPortRaw) {
    throw new Error('VITE_DEV_PORT must be set')
  }

  const devPort = Number(devPortRaw)
  if (!Number.isInteger(devPort) || devPort < 1 || devPort > 65535) {
    throw new Error(`VITE_DEV_PORT is invalid: ${devPortRaw}`)
  }

  return {
    plugins: [react()],
    server: {
      host: devHost,
      port: devPort,
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
