import { defineConfig, loadEnv } from "vite";

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), "");
  const device = env.COS_DEVICE || "http://192.168.0.187/";

  return {
    server: {
      proxy: {
        "/api": {
          target: device,
          changeOrigin: true,
        },
      },
    },
  };
});
