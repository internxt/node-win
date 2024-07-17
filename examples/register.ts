import VirtualDrive from "../src/virtual-drive";
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
import { IQueueManager, QueueItem } from "src";

const drive = new VirtualDrive(settings.syncRootPath, settings.defaultLogPath);

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

const fileCreatedAt = Date.now() - 172800000;
const fileUpdatedAt = Date.now() - 86400000;
const folderCreatedAt = Date.now() - 259200000;
const folderUpdatedAt = Date.now() - 345600000;

try {
  // creating files
  drive.createFileByPath(
    `/A (5th copy).pdfs`,
    "485df926-c07c-4a4c-a23b-e2b8e8d41988",
    1000,
    fileCreatedAt,
    fileUpdatedAt
  );

  drive.createFolderByPath(
    `/carpetaX`,
    "folder-f134-4dbd-8722-8f813ec412cf",
    1000,
    folderCreatedAt,
    folderUpdatedAt
  );

  drive.createFileByPath(
    `/carpetaX/file1.txt`,
    "8bb39aa1-4791-4505-998d-7d47379d10e4",
    1000,
    fileCreatedAt,
    fileUpdatedAt
  );

  drive.createFolderByPath(
    `/anyname`,
    "192391023-4791-4505-998d-7d47379d10e4",
    1000,
    folderCreatedAt,
    folderUpdatedAt
  );

  drive.createFileByPath(
    `/anyname/file2.txt`,
    "8eeadacd-6c94-40b4-b78c-067370386e95",
    1000,
    fileCreatedAt,
    fileUpdatedAt
  );
  drive.createFileByPath(
    `/fakefile.txt`,
    "8d2d85b1-b725-42eb-ac46-d5b2a90c57ea",
    57,
    fileCreatedAt,
    fileUpdatedAt
  );
  drive.createFileByPath(
    `/imagen.rar`,
    "03f7c464-3cf9-4664-9a79-078440cfac41",
    80582195,
    fileCreatedAt,
    fileUpdatedAt
  );
  drive.createFileByPath(
    `/noExtensionFile`,
    "8fd4be85-2c1b-4094-b67e-9adda53bdb0f",
    33020,
    fileCreatedAt,
    fileUpdatedAt
  );

  // creating folders
  drive.createFolderByPath(
    `/only-folder`,
    "3c598981-bca6-40ca-b016-fe2c0cdc5baf",
    1000,
    folderCreatedAt,
    folderUpdatedAt
  );
  drive.createFolderByPath(
    `/folderT`,
    "folder-f134-4dbd-8722-8f813ec412cf",
    1000,
    folderCreatedAt,
    folderUpdatedAt
  );

  drive.createFolderByPath(
    `/folderT/folderA`,
    "folder-54343-4dbd-8722-8f813ec412cf",
    1000,
    folderCreatedAt,
    folderUpdatedAt
  );

  drive.createFolderByPath(
    `/folderT/folderA/folderB`,
    "folder-465t-4dbd-8722-8f813ec412cf",
    1000,
    folderCreatedAt,
    folderUpdatedAt
  );

  drive.createFolderByPath(
    `/F.O.L.D.E.R`,
    "2d6bce9b-d006-412d-9a98-8c5fac2ad6e5",
    1000,
    folderCreatedAt,
    folderUpdatedAt
  );

  drive.createFolderByPath(
    `/folderWithFolder`,
    `folder-8a0e-43cb-805b-2719a686358f`,
    1000,
    folderCreatedAt,
    folderUpdatedAt
  );

  drive.createFolderByPath(
    `/folderWithFolder/folder2`,
    "f706369a-8a0e-43cb-805b-2719a686358f",
    1000,
    folderCreatedAt,
    folderUpdatedAt
  );
  drive.createFolderByPath(
    `/folderWithFolder/F.O.L.D.E.R`,
    "e7fd7e64-08e2-4bb0-8e36-ac01f7fe17d1",
    1000,
    folderCreatedAt,
    folderUpdatedAt
  );

  // create items
  drive.createItemByPath(
    `/item-folder/`,
    "ad7585c7-f134-4dbd-8722-8f813ec412cf",
    1000,
    folderCreatedAt,
    folderUpdatedAt
  );
  drive.createItemByPath(
    `/imagen-item.rar`,
    "62654a1c-0137-4fc7-b271-2c62f0d6f9f5",
    33020,
    fileCreatedAt,
    fileUpdatedAt
  );
  // /folderWithFolder/F.O.L.D.E.R
  drive.createFileByPath(
    `/folderWithFolder/F.O.L.D.E.R/unaImagen.rar`,
    "62654a1c-0137-4fc7-b271-2c62f0d6f9f5",
    33020,
    fileCreatedAt,
    fileUpdatedAt
  );

  // this part of example emulate the server ( createFilesWithSize ) and a storage layer ( ItemsInfoManager )
  createFilesWithSize(settings.syncRootPath, settings.serverRootPath);

  const itemsManager = ItemsInfoManager.initialize().then((itemsManager) => {
    itemsManager.add({
      "485df926-c07c-4a4c-a23b-e2b8e8d41988": `${settings.serverRootPath}\\A (5th copy).pdfs`,
      // "8bb39aa1-4791-4505-998d-7d47379d10e4": `${settings.serverRootPath}\\file1.txt`,
      "8eeadacd-6c94-40b4-b78c-067370386e95": `${settings.serverRootPath}\\file2.txt`,
      "8d2d85b1-b725-42eb-ac46-d5b2a90c57ea": `${settings.serverRootPath}\\fakefile.txt`,
      "03f7c464-3cf9-4664-9a79-078440cfac41": `${settings.serverRootPath}\\imagen.rar`,
      "8fd4be85-2c1b-4094-b67e-9adda53bdb0f": `${settings.serverRootPath}\\noExtensionFile`,
      "62654a1c-0137-4fc7-b271-2c62f0d6f9f5": `${settings.serverRootPath}\\imagen-item.rar`,
    });
  });

  const success = drive.convertToPlaceholder(
    settings.syncRootPath + "/imagen.rar",
    "03f7c464-3cf9-4664-9a79-sdffsd45423"
  );

  console.log(success);

  const success2 = drive.convertToPlaceholder(
    settings.syncRootPath + "/only-folder",
    "3c598981-bca6-40ca-b016-fe2c0cdc5baf"
  );

  //  [Get FileIdentity] testing function to get the file identity of a file or folder
  drive
    .getFileIdentity("/folderWithFolder/F.O.L.D.E.R")
    .then((fileIdentity) => {
      console.log("ID " + fileIdentity);
      console.log("count " + String(fileIdentity).length);
    });

  drive.getFileIdentity("/folderWithFolder").then((fileIdentity) => {
    console.log("ID " + fileIdentity);
    console.log("count " + String(fileIdentity).length);
  });

  //  [Get] file identity for file called unaImagen.rar
  drive
    .getFileIdentity("/folderWithFolder/F.O.L.D.E.R/unaImagen.rar")
    .then((fileIdentity) => {
      console.log("ID " + fileIdentity);
      console.log("count " + String(fileIdentity).length);
    });

  // Sleep for 5 seconds
  console.log("Sleeping for 5 seconds");
  setTimeout(() => {
    console.log("Woke up after 5 seconds");
    drive
      .getFileIdentity("/folderWithFolder/F.O.L.D.E.R/unaImagen.rar")
      .then((fileIdentity) => {
        console.log("ID " + fileIdentity);
        console.log("count " + String(fileIdentity).length);
      });
  }, 5000);

  //  [Update] file identity for file called unaImagen.rar

  console.log("UPDATE ID ");

  drive.updateFileIdentity(
    "/folderWithFolder/F.O.L.D.E.R/unaImagen.rar",
    "otroid12321",
    false
  );

  //  [Get Updated] file identity for file called unaImagen.rar
  drive
    .getFileIdentity("/folderWithFolder/F.O.L.D.E.R/unaImagen.rar")
    .then((fileIdentity) => {
      console.log("ID " + fileIdentity);
      console.log("count " + String(fileIdentity).length);
    });

  console.log(success2);

  drive.watchAndWait(
    settings.syncRootPath,
    queueManager,
    settings.watcherLogPath
  );
} catch (error) {
  drive.disconnectSyncRoot();
  VirtualDrive.unregisterSyncRoot(settings.syncRootPath);
  console.log("[EXAMPLE] error: " + error);
}

export default drive;
