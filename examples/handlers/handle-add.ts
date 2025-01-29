import { drive } from "examples/drive";
import { addInfoItem } from "examples/info-items-manager";

import { logger } from "@/logger";
import { QueueItem } from "@/queue/queueManager";

export const handleAdd = async (task: QueueItem) => {
  try {
    logger.info({ fn: "handleAdd", path: task.path });
    const id = addInfoItem(task.path);
    drive.convertToPlaceholder(task.path, id);
  } catch (error) {
    logger.error(error, "handleAdd");
  }
};
