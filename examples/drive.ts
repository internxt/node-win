import { createLogger } from "@/logger";
import VirtualDrive from "@/virtual-drive";

import settings from "./settings";

export const drive = new VirtualDrive(settings.syncRootPath, settings.providerid, settings.defaultLogPath);
export const logger = createLogger(settings.defaultLogPath);
