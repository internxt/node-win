import { drive } from "./drive";
import settings from "./settings";
import {
  onCancelFetchDataCallback,
  onDeleteCallbackWithCallback,
  onFetchDataCallback,
  onFileAddedCallback,
  onMessageCallback,
  onRenameCallbackWithCallback,
} from "./callbacks";
import { ItemsInfoManager, createFilesWithSize } from "./utils";
import { QueueManager } from "./queueManager";
import { IQueueManager, QueueItem, VirtualDrive } from "src";
import { generateRandomFilesAndFolders } from "./utils/generate-random-file-tree";

console.log("Registering sync root: " + settings.syncRootPath);

drive.registerSyncRoot(
  settings.driveName,
  settings.driveVersion,
  "{12345678-1234-1234-1234-123456789012}",
  {
    notifyDeleteCallback: onDeleteCallbackWithCallback,
    notifyRenameCallback: onRenameCallbackWithCallback,
    notifyFileAddedCallback: onFileAddedCallback,
    fetchDataCallback: onFetchDataCallback,
    cancelFetchDataCallback: onCancelFetchDataCallback,
    notifyMessageCallback: onMessageCallback,
  },
  settings.defaultIconPath
);

const handlerAdd = async (task: QueueItem) => {
  try {
    console.log("[EXAMPLE] File added in callback: " + task.path);
    await new Promise((resolve) =>
      setTimeout(() => {
        resolve(undefined);
      }, 1000)
    );
    const result = Math.random().toString(36).substring(2, 7);
    await drive.convertToPlaceholder(task.path, result);
    await drive.updateSyncStatus(task.path, task.isFolder, true);
  } catch (error) {
    console.error(error);
  }
};

const handleDehydrate = async (task: QueueItem) => {
  try {
    console.log("[EXAMPLE] File dehydrated in callback: " + task.path);
    await new Promise((resolve) =>
      setTimeout(() => {
        resolve(undefined);
      }, 1000)
    );
    console.log("Dehydrating file: " + task.path);
    await drive.dehydrateFile(task.path);
  } catch (error) {
    console.error(error);
  }
};

const handleHydrate = async (task: QueueItem) => {
  try {
    console.log("[EXAMPLE] File hydrated in callback: " + task.path);
    await new Promise((resolve) =>
      setTimeout(() => {
        resolve(undefined);
      }, 1000)
    );

    const tempPath = task.path.replace(
      settings.syncRootPath,
      settings.serverRootPath
    );

    console.log("Hydrating file: " + task.path);
    // await drive.transferData(tempPath, task.path);

    console.log("[EXAMPLE] File trasnfer in callback: " + task.path);
    await new Promise((resolve) =>
      setTimeout(() => {
        resolve(undefined);
      }, 1000)
    );
  } catch (error) {
    console.error(error);
  }
};

const handleChangeSize = async (task: QueueItem) => {
  try {
    console.log("[EXAMPLE] File size changed in callback: " + task.path);
    await new Promise((resolve) =>
      setTimeout(() => {
        resolve(undefined);
      }, 1000)
    );
    const result = Math.random().toString(36).substring(2, 7);
    await drive.convertToPlaceholder(task.path, result);
    await drive.updateFileIdentity(task.path, result, false);
    await drive.updateSyncStatus(task.path, task.isFolder, true);
    // await drive.updateFileSize(task.path);
  } catch (error) {
    console.error(error);
  }
};

const queueManager: IQueueManager = new QueueManager({
  handleAdd: handlerAdd,
  handleHydrate: handleHydrate,
  handleDehydrate: handleDehydrate,
  handleChangeSize: handleChangeSize,
});

drive.connectSyncRoot();

const fileGenerationOptions = {
  rootPath: '',
  depth: 3,
  filesPerFolder: 3,
  foldersPerLevel: 3,
  meanSize: 5000000,
  stdDev: 6000000,
};

(async () => {
  try {
    const fileMap = await generateRandomFilesAndFolders(drive, fileGenerationOptions);

    createFilesWithSize(settings.syncRootPath, settings.serverRootPath);

    const itemsManager = await ItemsInfoManager.initialize()

    for (const key in fileMap) {
      const value = fileMap[key];
      fileMap[key] = settings.serverRootPath + value.replace("/", "\\");
    }
    
    await itemsManager.add(fileMap);

    drive.watchAndWait(
      settings.syncRootPath,
      queueManager,
      settings.watcherLogPath
    );

    console.log("Proceso de generación y vigilancia completado.");
  } catch (error) {
    drive.disconnectSyncRoot();
    VirtualDrive.unregisterSyncRoot(settings.syncRootPath);
    console.log("[EXAMPLE] error: " + error);
  }
})();

export default drive;
