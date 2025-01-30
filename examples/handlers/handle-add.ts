import { drive } from "examples/drive";
import { addInfoItem } from "examples/info-items-manager";

import { logger } from "@/logger";
import { QueueItem } from "@/queue/queueManager";
import { v4 } from "uuid";

export const handleAdd = async (task: QueueItem) => {
  try {
    logger.info({ fn: "handleAdd", task });
    const id = task.isFolder ? v4() : addInfoItem(task.path);
    drive.convertToPlaceholder(task.path, id);
  } catch (error) {
    logger.error("handleAdd", error);
  }
};
