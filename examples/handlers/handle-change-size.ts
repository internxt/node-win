import { drive } from "examples/drive";

import { logger } from "@/logger";
import { QueueItem } from "@/queue/queueManager";

export const handleChangeSize = async (task: QueueItem) => {
  try {
    logger.info({ fn: "handleChangeSize", path: task.path });
    const result = Math.random().toString(36).substring(2, 7);
    drive.convertToPlaceholder(task.path, result);
    drive.updateFileIdentity(task.path, result, false);
    drive.updateSyncStatus(task.path, task.isFolder, true);
  } catch (error) {
    logger.error(error, "handleChangeSize");
  }
};
