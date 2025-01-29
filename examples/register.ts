import { logger } from "@/logger";
import { QueueManager } from "@/queue/queue-manager";
import VirtualDrive from "@/virtual-drive";

import { onCancelFetchDataCallback, onMessageCallback, onRenameCallbackWithCallback } from "./callbacks";
import { notifyDeleteCallback } from "./callbacks/notify-delete.callback";
import { fetchDataCallback } from "./callbacks/notify-fetch-data.callback";
import { drive } from "./drive";
import { handleAdd } from "./handlers/handle-add";
import { handleChangeSize } from "./handlers/handle-change-size";
import { handleDehydrate } from "./handlers/handle-dehydrate";
import { handleHydrate } from "./handlers/handle-hydrate";
import { initInfoItems } from "./info-items-manager";
import settings from "./settings";

logger.info("Registering sync root: " + settings.syncRootPath);

const callbacks = {
  notifyDeleteCallback,
  notifyRenameCallback: onRenameCallbackWithCallback,
  fetchDataCallback,
  cancelFetchDataCallback: onCancelFetchDataCallback,
  notifyMessageCallback: onMessageCallback,
};

const handlers = { handleAdd, handleHydrate, handleDehydrate, handleChangeSize };
const notify = { onTaskSuccess: async () => undefined, onTaskProcessing: async () => undefined };
const queueManager = new QueueManager(handlers, notify, settings.queuePersistPath);

drive.registerSyncRoot(settings.driveName, settings.driveVersion, settings.providerid, callbacks, settings.iconPath);
drive.connectSyncRoot();

try {
  initInfoItems();
  drive.watchAndWait(settings.syncRootPath, queueManager, settings.watcherLogPath);
} catch (error) {
  console.error(error);
  drive.disconnectSyncRoot();
  VirtualDrive.unregisterSyncRoot(settings.syncRootPath);
}
