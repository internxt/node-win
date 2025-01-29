import { QueueItem, VirtualDrive } from "src";

import { logger } from "@/logger";

import { onCancelFetchDataCallback, onMessageCallback, onRenameCallbackWithCallback } from "./callbacks";
import { onDeleteCallback } from "./callbacks/notify-delete.callback";
import { onFetchDataCallback } from "./callbacks/notify-fetch-data.callback";
import { drive } from "./drive";
import { addInfoItem, initInfoItems } from "./info-items-manager";
import { QueueManager } from "./queueManager";
import settings from "./settings";
import { generateRandomFilesAndFolders } from "./utils/generate-random-file-tree";

logger.info("Registering sync root: " + settings.syncRootPath);

drive.registerSyncRoot(
  settings.driveName,
  settings.driveVersion,
  "{12345678-1234-1234-1234-123456789012}",
  {
    notifyDeleteCallback: onDeleteCallback,
    notifyRenameCallback: onRenameCallbackWithCallback,
    fetchDataCallback: onFetchDataCallback,
    cancelFetchDataCallback: onCancelFetchDataCallback,
    notifyMessageCallback: onMessageCallback,
  },
  settings.defaultIconPath,
);

const handleAdd = async (task: QueueItem) => {
  try {
    logger.info({ fn: "handleAdd", path: task.path });
    const id = await addInfoItem(task.path);
    drive.convertToPlaceholder(task.path, id);
    // await drive.updateSyncStatus(task.path, task.isFolder, true);
  } catch (error) {
    logger.error(error, "handleAdd");
  }
};

const handleDehydrate = async (task: QueueItem) => {
  try {
    logger.info({ fn: "handleDehydrate", path: task.path });
    drive.dehydrateFile(task.path);
  } catch (error) {
    logger.error(error, "handleDehydrate");
  }
};

const handleHydrate = async (task: QueueItem) => {
  try {
    logger.info({ fn: "handleHydrate", path: task.path });
    await drive.hydrateFile(task.path);
  } catch (error) {
    logger.error(error, "handleHydrate");
  }
};

const handleChangeSize = async (task: QueueItem) => {
  try {
    logger.info({ fn: "handleChangeSize", path: task.path });
    const result = Math.random().toString(36).substring(2, 7);
    drive.convertToPlaceholder(task.path, result);
    drive.updateFileIdentity(task.path, result, false);
    await drive.updateSyncStatus(task.path, task.isFolder, true);
    // await drive.updateFileSize(task.path);
  } catch (error) {
    logger.error(error, "handleChangeSize");
  }
};

const queueManager = new QueueManager({
  handleAdd,
  handleHydrate,
  handleDehydrate,
  handleChangeSize,
});

drive.connectSyncRoot();

const fileGenerationOptions = {
  rootPath: "",
  depth: 3,
  filesPerFolder: 3,
  foldersPerLevel: 3,
  meanSize: 5000000,
  stdDev: 6000000,
};

(async () => {
  try {
    await initInfoItems();
    // const fileMap = await generateRandomFilesAndFolders(drive, fileGenerationOptions);
    drive.watchAndWait(settings.syncRootPath, queueManager, settings.watcherLogPath);
  } catch (error) {
    drive.disconnectSyncRoot();
    VirtualDrive.unregisterSyncRoot(settings.syncRootPath);
    console.log("[EXAMPLE] error: " + error);
  }
})();

export default drive;
