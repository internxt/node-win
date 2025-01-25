import "dotenv/config";
import path, { join } from "path";
import { cwd } from "process";
import { z } from "zod";

function sanitizePath(filePath: string): string {
  return filePath.replace(/\n/g, "\\n");
}

function normalizePath(filePath: string) {
  return sanitizePath(filePath.replace(/\\n/g, "\\n").replace(/\\/g, path.sep));
}

const envVarsSchema = z.object({
  EXAMPLE_DRIVE_NAME: z.string().min(1),
  EXAMPLE_DRIVE_VERSION: z.string().min(1),
  EXAMPLE_SYNC_ROOT_PATH: z.string().min(1),
  EXAMPLE_DEFAULT_LOG_PATH: z.string().min(1),
  EXAMPLE_SERVER_ROOT_PATH: z.string().min(1),
  EXAMPLE_WATCHER_LOG_PATH: z.string().min(1),
});

const { data: envVars, error } = envVarsSchema.safeParse(process.env);

if (error) {
  throw new Error(`Error de configuración: ${error.message}`);
}

const settings = {
  driveName: envVars.EXAMPLE_DRIVE_NAME,
  driveVersion: envVars.EXAMPLE_DRIVE_VERSION,
  syncRootPath: normalizePath(envVars.EXAMPLE_SYNC_ROOT_PATH),
  defaultLogPath: normalizePath(envVars.EXAMPLE_DEFAULT_LOG_PATH),
  defaultIconPath: join(cwd(), "assets", "icon.ico"),
  serverRootPath: normalizePath(envVars.EXAMPLE_SERVER_ROOT_PATH),
  watcherLogPath: normalizePath(envVars.EXAMPLE_WATCHER_LOG_PATH),
};

export default settings;
