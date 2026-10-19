import { defineConfig, loadEnv } from 'vite'
import react from '@vitejs/plugin-react'
import { fileURLToPath } from 'node:url'
import { dirname } from 'node:path'

const __dirname = dirname(fileURLToPath(import.meta.url))

// https://vite.dev/config/
export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), '')
  // Proxy target only. Deliberately NOT a VITE_ variable: anything with that
  // prefix is baked into the client bundle, and the client posting to an
  // absolute URL is what silently breaks login (the request succeeds, the
  // session cookie is dropped as cross-origin). This lets a second instance --
  // a public demo against a copied database, say -- point at a different
  // backend while the browser still talks to its own origin.
  const proxyTarget = process.env.DEBATE_PROXY_TARGET?.trim()
  const apiUrl = proxyTarget || env.VITE_API_URL?.trim() || 'http://localhost:8080'

  return {
    root: __dirname,
    plugins: [react()],
    resolve: {
      alias: {
        "@bufbuild/protobuf/codegenv2": fileURLToPath(
          new URL("./node_modules/@bufbuild/protobuf/dist/esm/codegenv2/index.js", import.meta.url)
        ),
      },
    },
    server: {
      host: "0.0.0.0",
      port: 5173,
      strictPort: true,
      // Vite 7 rejects requests whose Host header it does not recognise, which
      // is every request arriving through a tunnel -- they come in as
      // <random>.trycloudflare.com or <random>.loca.lt, not localhost. Without
      // these the tunnel connects fine and then serves "Blocked request" for
      // every page, which reads as a tunnel fault rather than a Vite policy.
      allowedHosts: ['.trycloudflare.com', '.loca.lt', '.lhr.life', '.ngrok-free.app', '.ngrok.io'],
      proxy: {
        '/api': {
          target: apiUrl,
          changeOrigin: true,
          rewrite: (path) => path.replace(/^\/api/, ''),
        },
      },
      historyApiFallback: true
    },
    build: {
      rollupOptions: {
        output: {
          manualChunks: undefined
        }
      }
    }
  }
})
