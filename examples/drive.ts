import VirtualDrive from "@/virtual-drive";

import settings from "./settings";

export const drive = new VirtualDrive(settings.syncRootPath, settings.defaultLogPath);
export const logger = drive.logger;
