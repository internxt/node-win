import { drive, logger } from "examples/drive";

import { QueueItem } from "@/queue/queueManager";

export const handleDehydrate = async (task: QueueItem) => {
  try {
    logger.info({ fn: "handleDehydrate", path: task.path });
    drive.dehydrateFile(task.path);
  } catch (error) {
    logger.error("handleDehydrate", error);
  }
};
