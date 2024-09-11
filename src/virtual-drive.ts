import path from "path";
import fs from "fs";
import { deleteAllSubfolders } from "./utils";
import { Worker } from "worker_threads";
import { Watcher } from "./watcher/watcher";
import { ExtraCallbacks, InputSyncCallbacks } from "./types/callbacks.type";
import { Status } from "./types/placeholder.type";
import { IQueueManager } from "./queue/queueManager";

const addon = require("../../build/Release/addon.node");
interface ItemInfo {
  path: string;
  fileIdentity: string;
  isPlaceholder: boolean;
}
interface Addon {
  connectSyncRoot(path: string): any;
  createPlaceholderFile(
    fileName: string,
    fileId: string,
    fileSize: number,
    combinedAttributes: number,
    creationTime: string,
    lastWriteTime: string,
    lastAccessTime: string,
    path: string
  ): any;
  registerSyncRootWindowsStorageProvider(
    path: string,
    providerName: string,
    providerVersion: string,
    providerId: string
  ): any;
  unregisterSyncRoot(path: string): any;
  watchAndWait(path: string): any;
  getItems(): any;
}

type Callbacks = InputSyncCallbacks & ExtraCallbacks;
class VirtualDrive {
  PLACEHOLDER_ATTRIBUTES: { [key: string]: number };
  syncRootPath: string;
  callbacks?: Callbacks;

  // private watcherBuilder: WatcherBuilder;
  private watcher: Watcher;

  constructor(syncRootPath: string, loggerPath?: string) {
    this.PLACEHOLDER_ATTRIBUTES = {
      FILE_ATTRIBUTE_READONLY: 0x1,
      FILE_ATTRIBUTE_HIDDEN: 0x2,
      FOLDER_ATTRIBUTE_READONLY: 0x1,
    };

    this.watcher = Watcher.Instance;

    this.syncRootPath = syncRootPath;
    this.createSyncRootFolder();

    let pathElements = this.syncRootPath.split("\\\\");
    pathElements.pop();
    let parentPath = pathElements.join("\\\\");

    this.addLoggerPath(loggerPath ?? parentPath);
  }

  addLoggerPath(loggerPath: string) {
    console.log("loggerPath: ", loggerPath);
    addon.addLoggerPath(loggerPath);
  }

  getPlaceholderState(path: string): Status {
    return addon.getPlaceholderState(this.syncRootPath + path);
  }

  getPlaceholderWithStatePending(): any {
    return addon.getPlaceholderWithStatePending(this.syncRootPath);
  }

  getInputSyncCallbacks(): InputSyncCallbacks {
    if (this.callbacks === undefined) {
      throw new Error("Callbacks are not defined");
    }

    const inputSyncCallbackKeys: (keyof InputSyncCallbacks)[] = [
      "fetchDataCallback",
      "validateDataCallback",
      "cancelFetchDataCallback",
      "fetchPlaceholdersCallback",
      "cancelFetchPlaceholdersCallback",
      "notifyFileOpenCompletionCallback",
      "notifyFileCloseCompletionCallback",
      "notifyDehydrateCallback",
      "notifyDehydrateCompletionCallback",
      "notifyDeleteCallback",
      "notifyDeleteCompletionCallback",
      "notifyRenameCallback",
      "notifyRenameCompletionCallback",
      "noneCallback",
    ];

    const result: InputSyncCallbacks = {};

    for (const key of inputSyncCallbackKeys) {
      if (this.callbacks[key] !== undefined) {
        result[key] = this.callbacks[key];
      }
    }

    return result;
  }

  getExtraCallbacks(): ExtraCallbacks {
    const extraCallbackKeys: (keyof ExtraCallbacks)[] = [
      "notifyFileAddedCallback",
      "notifyMessageCallback",
    ];

    const result: ExtraCallbacks = {};
    if (this.callbacks === undefined) {
      throw new Error("Callbacks are not defined");
    }

    for (const key of extraCallbackKeys) {
      if (this.callbacks[key] !== undefined) {
        result[key] = this.callbacks[key];
      }
    }

    return result;
  }

  createSyncRootFolder() {
    if (!fs.existsSync(this.syncRootPath)) {
      fs.mkdirSync(this.syncRootPath, { recursive: true });
    }
  }

  convertToWindowsTime(jsTime: number): bigint {
    return BigInt(jsTime) * 10000n + 116444736000000000n;
  }

  async getItemsIds(): Promise<ItemInfo[]> {
    console.log("getItemsIdsSync");
    return addon.getItemsIds(this.syncRootPath);
  }

  async getFileIdentity(relativePath: string): Promise<string> {
    const fullPath = path.join(this.syncRootPath, relativePath);
    return addon.getFileIdentity(fullPath);
  }

  async deleteFileSyncRoot(relativePath: string): Promise<void> {
    const fullPath = path.join(this.syncRootPath, relativePath);
    return addon.deleteFileSyncRoot(fullPath);
  }

  async connectSyncRoot(): Promise<any> {
    return await addon.connectSyncRoot(
      this.syncRootPath,
      this.getInputSyncCallbacks()
    );
  }

  createPlaceholderFile(
    fileName: string,
    fileId: string,
    fileSize: number,
    fileAttributes: number,
    creationTime: number,
    lastWriteTime: number,
    lastAccessTime: number,
    basePath: string
  ): any {
    const combinedAttributes = fileAttributes;
    const creationTimeStr = this.convertToWindowsTime(creationTime).toString();
    const lastWriteTimeStr =
      this.convertToWindowsTime(lastWriteTime).toString();
    const lastAccessTimeStr =
      this.convertToWindowsTime(lastAccessTime).toString();

    return addon.createPlaceholderFile(
      fileName,
      fileId,
      fileSize,
      combinedAttributes,
      creationTimeStr,
      lastWriteTimeStr,
      lastAccessTimeStr,
      basePath
    );
  }

  createPlaceholderDirectory(
    itemName: string,
    itemId: string,
    isDirectory: boolean,
    itemSize: number,
    fileAttributes: number,
    creationTime: number,
    lastWriteTime: number,
    lastAccessTime: number,
    path: string
  ) {
    const creationTimeStr = this.convertToWindowsTime(creationTime).toString();
    const lastWriteTimeStr =
      this.convertToWindowsTime(lastWriteTime).toString();
    return addon.createEntry(
      itemName,
      itemId,
      isDirectory,
      itemSize,
      fileAttributes,
      creationTimeStr,
      lastWriteTimeStr,
      lastAccessTime,
      path
    );
  }

  private isValidFolderPath(path: string) {
    return path.startsWith("/") && path.endsWith("/") && !path.includes(".");
  }

  private isValidFilePath(path: string) {
    return path.includes(".");
  }

  async registerSyncRoot(
    providerName: string,
    providerVersion: string,
    providerId: string,
    callbacks: Callbacks,
    logoPath: string
  ): Promise<any> {
    this.callbacks = callbacks;
    return await addon.registerSyncRoot(
      this.syncRootPath,
      providerName,
      providerVersion,
      providerId,
      logoPath
    );
  }

  static unregisterSyncRoot(syncRootPath: string): any {
    const result = addon.unregisterSyncRoot(syncRootPath);
    return result;
  }

  private test(): void {
    console.log("Test");
  }

  watchAndWait(
    path: string,
    queueManager: IQueueManager,
    loggerPath: string
  ): void {
    // if (this.callbacks === undefined) {
    //   throw new Error("Callbacks are not defined");
    // }

    this.watcher.queueManager = queueManager;

    this.watcher.logPath = loggerPath;

    this.watcher.syncRootPath = path;
    this.watcher.options = {
      ignored: /(^|[\/\\])\../,
      persistent: true,
      ignoreInitial: true,
      followSymlinks: true,
      depth: undefined,
      awaitWriteFinish: {
        stabilityThreshold: 2000,
        pollInterval: 100,
      },
      usePolling: true,
    };

    this.watcher.virtualDriveFunctions = {
      CfAddItem: this.test,
      CfDehydrate: this.test,
      CfHydrate: this.test,
      CfNotifyMessage: this.test,
      CfUpdateItem: this.test,
      CfGetPlaceHolderAttributes: addon.getPlaceholderAttribute,
      CfUpdateSyncStatus: addon.updateSyncStatus,
      CfGetPlaceHolderIdentity: addon.getFileIdentity,
      CfGetPlaceHolderState: addon.getPlaceholderState,
      CfConverToPlaceholder: addon.convertToPlaceholder,
    };

    this.watcher.watchAndWait();
    // addon.watchAndWait(path, this.getExtraCallbacks());
  }

  createFileByPath(
    relativePath: string,
    itemId: string,
    size: number = 0,
    creationTime: number = Date.now(),
    lastWriteTime: number = Date.now()
  ): void {
    const fullPath = path.join(this.syncRootPath, relativePath);
    const splitPath = relativePath.split("/").filter((p) => p);
    const directoryPath = path.resolve(this.syncRootPath);
    let currentPath = directoryPath;
    try {
      for (let i = 0; i < splitPath.length - 1; i++) {
        // everything except last element
        const dir = splitPath[i];

        currentPath = path.join(currentPath, dir);
      }
      // last element is the file
      this.createPlaceholderFile(
        path.basename(fullPath),
        itemId,
        size,
        this.PLACEHOLDER_ATTRIBUTES.FILE_ATTRIBUTE_NORMAL,
        creationTime,
        lastWriteTime,
        Date.now(),
        currentPath
      );
    } catch (error) {
      //@ts-ignore
      console.error(`Error al crear placeholder: ${error.message}`);
    }
  }

  createFolderByPath(
    relativePath: string,
    itemId: string,
    size: number = 0,
    creationTime: number = Date.now(),
    lastWriteTime: number = Date.now()
  ): void {
    const splitPath = relativePath.split("/").filter((p) => p);
    const directoryPath = path.resolve(this.syncRootPath);
    let currentPath = directoryPath;
    // solo crear el ultimo directorio
    for (let i = 0; i < splitPath.length; i++) {
      const dir = splitPath[i];
      const last = i === splitPath.length - 1;
      if (last) {
        if (fs.existsSync(currentPath)) {
          this.createPlaceholderDirectory(
            dir,
            itemId,
            true,
            size,
            this.PLACEHOLDER_ATTRIBUTES.FILE_ATTRIBUTE_NORMAL,
            creationTime,
            lastWriteTime,
            Date.now(),
            currentPath
          );
        }
      }
      currentPath = path.join(currentPath, dir);
    }
  }

  createItemByPath(
    relativePath: string,
    itemId: string,
    size: number = 0,
    creationTime: number = Date.now(),
    lastWriteTime: number = Date.now()
  ): void {
    const fullPath = path.join(this.syncRootPath, relativePath);
    const splitPath = relativePath.split("/").filter((p) => p);
    const directoryPath = path.resolve(this.syncRootPath);
    let currentPath = directoryPath;
    if (this.isValidFolderPath(relativePath)) {
      // Es un directorio

      for (const dir of splitPath) {
        if (fs.existsSync(currentPath)) {
          try {
            this.createPlaceholderDirectory(
              dir,
              itemId,
              true,
              size,
              this.PLACEHOLDER_ATTRIBUTES.FILE_ATTRIBUTE_NORMAL,
              creationTime,
              lastWriteTime,
              Date.now(),
              currentPath
            );
          } catch (error) {
            //@ts-ignore
            console.error(`Error while creating directory: ${error.message}`);
          }
        }
        currentPath = path.join(currentPath, dir);
      }
    } else if (this.isValidFilePath(relativePath)) {
      // Es un archivo

      try {
        for (let i = 0; i < splitPath.length - 1; i++) {
          // everything except last element
          const dir = splitPath[i];
          if (fs.existsSync(currentPath)) {
            this.createPlaceholderDirectory(
              dir,
              itemId,
              true,
              0,
              this.PLACEHOLDER_ATTRIBUTES.FILE_ATTRIBUTE_NORMAL,
              Date.now(),
              Date.now(),
              Date.now(),
              currentPath
            );
          }
          currentPath = path.join(currentPath, dir);
        }
        // last element is the file
        this.createPlaceholderFile(
          path.basename(fullPath),
          itemId,
          size,
          this.PLACEHOLDER_ATTRIBUTES.FILE_ATTRIBUTE_NORMAL,
          creationTime,
          lastWriteTime,
          Date.now(),
          currentPath
        );
      } catch (error) {
        //@ts-ignore
        console.error(`Error al crear placeholder: ${error.message}`);
      }
    } else {
      console.error("Invalid path");
    }
  }

  disconnectSyncRoot(): any {
    return addon.disconnectSyncRoot(this.syncRootPath);
  }

  async updateSyncStatus(
    itemPath: string,
    isDirectory: boolean,
    sync: boolean = true
  ): Promise<void> {
    const isRelative = !itemPath.includes(this.syncRootPath);

    if (isRelative) {
      itemPath = path.join(this.syncRootPath, itemPath);
    }
    return await addon.updateSyncStatus(itemPath, sync, isDirectory);
  }

  convertToPlaceholder(itemPath: string, id: string): boolean {
    return addon.convertToPlaceholder(itemPath, id);
  }
  updateFileIdentity(itemPath: string, id: string, isDirectory: boolean): void {
    if (!itemPath.includes(this.syncRootPath)) {
      itemPath = path.join(this.syncRootPath, itemPath);
    }
    addon.updateFileIdentity(itemPath, id, isDirectory);
  }

  closeDownloadMutex(): void {
    return addon.closeMutex();
  }

  async dehydrateFile(itemPath: string): Promise<void> {
    return await addon.dehydrateFile(itemPath);
  }

  async hydrateFile(itemPath: string): Promise<void> {
    return await addon.hydrateFile(itemPath);
  }

  async getPlaceholderAttribute(itemPath: string): Promise<any> {
    return await addon.getPlaceholderAttribute(itemPath);
  }

  async isTempFile(itemPath: string): Promise<boolean> {
    return await addon.isTempFile(itemPath);
  }
}

export default VirtualDrive;
