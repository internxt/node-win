import "dotenv/config";
import Joi from "joi";
import path from "path";

function sanitizePath(filePath: string): string {
  return filePath.replace(/\n/g, '\\n');
}

function normalizePath(filePath: string) {
  return sanitizePath(filePath.replace(/\\n/g, "\\n").replace(/\\/g, path.sep));
}

const envVarsSchema = Joi.object({
  EXAMPLE_DRIVE_NAME: Joi.string().required(),
  EXAMPLE_DRIVE_VERSION: Joi.string().required(),
  EXAMPLE_SYNC_ROOT_PATH: Joi.string().required(),
  EXAMPLE_DEFAULT_LOG_PATH: Joi.string().required(),
  EXAMPLE_DEFAULT_ICON_PATH: Joi.string().required(),
  EXAMPLE_SERVER_ROOT_PATH: Joi.string().required(),
  EXAMPLE_WATCHER_LOG_PATH: Joi.string().required(),
}).unknown();

const { value: envVars, error } = envVarsSchema.validate(process.env);

if (error) {
  throw new Error(`Error de configuración: ${error.message}`);
}

const settings = {
  driveName: envVars.EXAMPLE_DRIVE_NAME,
  driveVersion: envVars.EXAMPLE_DRIVE_VERSION,
  syncRootPath: normalizePath(envVars.EXAMPLE_SYNC_ROOT_PATH),
  defaultLogPath: normalizePath(envVars.EXAMPLE_DEFAULT_LOG_PATH),
  defaultIconPath: normalizePath(envVars.EXAMPLE_DEFAULT_ICON_PATH),
  serverRootPath: normalizePath(envVars.EXAMPLE_SERVER_ROOT_PATH),
  watcherLogPath: normalizePath(envVars.EXAMPLE_WATCHER_LOG_PATH),
};

export default settings;
