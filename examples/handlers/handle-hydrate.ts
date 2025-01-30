import { drive } from "examples/drive";

import { logger } from "@/logger";
import { QueueItem } from "@/queue/queueManager";

export const handleHydrate = async (task: QueueItem) => {
  try {
    logger.info({ fn: "handleHydrate", path: task.path });
    await drive.hydrateFile(task.path);
  } catch (error) {
    logger.error("handleHydrate", error);
  }
};
