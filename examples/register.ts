import { QueueManager } from "@/queue/queue-manager";
import VirtualDrive from "@/virtual-drive";

import { cancelFetchDataCallback } from "./callbacks/cancel-fetch-data.callback";
import { notifyDeleteCallback } from "./callbacks/notify-delete.callback";
import { fetchDataCallback } from "./callbacks/notify-fetch-data.callback";
import { notifyMessageCallback } from "./callbacks/notify-message.callback";
import { notifyRenameCallback } from "./callbacks/notify-rename.callback";
import { drive, logger } from "./drive";
import { handleAdd } from "./handlers/handle-add";
import { handleChangeSize } from "./handlers/handle-change-size";
import { handleDehydrate } from "./handlers/handle-dehydrate";
import { handleHydrate } from "./handlers/handle-hydrate";
import { initInfoItems } from "./info-items-manager";
import settings from "./settings";

const callbacks = { notifyDeleteCallback, notifyRenameCallback, fetchDataCallback, cancelFetchDataCallback, notifyMessageCallback };
const handlers = { handleAdd, handleHydrate, handleDehydrate, handleChangeSize };

const notify = { onTaskSuccess: async () => undefined, onTaskProcessing: async () => undefined };
const queueManager = new QueueManager(handlers, notify, settings.queuePersistPath);

drive.registerSyncRoot(settings.driveName, settings.driveVersion, settings.providerid, callbacks, settings.iconPath);
drive.connectSyncRoot();

try {
  initInfoItems();
  drive.watchAndWait(settings.syncRootPath, queueManager, settings.watcherLogPath);
} catch (error) {
  logger.error(error);
  drive.disconnectSyncRoot();
  drive.unregisterSyncRoot();
}
