import { inspect } from "util";

import { TBody } from "@/logger";
import VirtualDrive from "@/virtual-drive";

import settings from "./settings";

const customInspect = (body: TBody) => {
  return inspect(body, { depth: Infinity, colors: true, breakLength: Infinity });
};

export const logger = {
  debug(body: TBody) {
    console.debug(customInspect(body));
  },
  info(body: TBody) {
    console.info(customInspect(body));
  },
  warn(body: TBody) {
    console.warn(customInspect(body));
  },
  error(body: TBody) {
    console.error(customInspect(body));
  },
};

export const drive = new VirtualDrive({
  syncRootPath: settings.syncRootPath,
  providerId: settings.providerid,
  loggerPath: settings.defaultLogPath,
  logger,
});
