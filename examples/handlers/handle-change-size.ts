import { drive, logger } from "examples/drive";
import { v4 } from "uuid";

import { QueueItem } from "@/queue/queueManager";

export const handleChangeSize = async (task: QueueItem) => {
  try {
    logger.info({ fn: "handleChangeSize", path: task.path });
    const id = v4();
    drive.convertToPlaceholder(task.path, id);
    drive.updateFileIdentity(task.path, id, false);
  } catch (error) {
    logger.error("handleChangeSize", error);
  }
};
