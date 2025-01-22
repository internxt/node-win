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
import { VirtualDrive } from "src";
import { generateRandomFilesAndFolders } from "./utils/generate-random-file-tree";
import { buildQueueManager } from "./build-queue-manager";

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

drive.connectSyncRoot();

const queueManager = buildQueueManager(drive);

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
