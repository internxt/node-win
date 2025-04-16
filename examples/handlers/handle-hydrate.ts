import { drive, logger } from "examples/drive";

import { QueueItem } from "@/queue/queueManager";

export const handleHydrate = async (task: QueueItem) => {
  try {
    logger.info({ fn: "handleHydrate", path: task.path });
    await drive.hydrateFile({
      itemPath: task.path,
    });
  } catch (error) {
    logger.error("handleHydrate", error);
  }
};
