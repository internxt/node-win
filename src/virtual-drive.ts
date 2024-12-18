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
  connectSyncRoot(path: string, callbacks?: InputSyncCallbacks): Promise<any>;
  createPlaceholderFile(
    fileName: string,
    fileId: string,
    fileSize: number,
    combinedAttributes: number,
    creationTime: string,
    lastWriteTime: string,
    lastAccessTime: string,
    basePath: string
  ): any;
  registerSyncRootWindowsStorageProvider(
    path: string,
    providerName: string,
    providerVersion: string,
    providerId: string
  ): any;
  unregisterSyncRoot(path: string): any;
  watchAndWait(path: string): any;
  getItemsIds(path: string): Promise<ItemInfo[]>;
  getFileIdentity(path: string): any;
  deleteFileSyncRoot(path: string): Promise<void>;
  addLoggerPath(path: string): void;
  getPlaceholderState(path: string): Status;
  getPlaceholderWithStatePending(path: string): any;
  registerSyncRoot(
    syncRootPath: string,
    providerName: string,
    providerVersion: string,
    providerId: string,
    logoPath: string
  ): Promise<any>;
  disconnectSyncRoot(path: string): any;
  updateSyncStatus(itemPath: string, sync: boolean, isDirectory: boolean): Promise<void>;
  convertToPlaceholder(itemPath: string, id: string): boolean;
  updateFileIdentity(itemPath: string, id: string, isDirectory: boolean): void;
  closeMutex(): void;
  dehydrateFile(itemPath: string): Promise<void>;
  hydrateFile(itemPath: string): Promise<void>;
  getPlaceholderAttribute(itemPath: string): Promise<any>;
  createEntry(
    itemName: string,
    itemId: string,
    isDirectory: boolean,
    itemSize: number,
    fileAttributes: number,
    creationTime: string,
    lastWriteTime: string,
    lastAccessTime: number,
    path: string
  ): any;
}

type Callbacks = InputSyncCallbacks & ExtraCallbacks;

class VirtualDrive {
  PLACEHOLDER_ATTRIBUTES: { [key: string]: number };
  syncRootPath: string;
  callbacks?: Callbacks;
  private watcher: Watcher;

  constructor(syncRootPath: string, loggerPath?: string) {
    this.PLACEHOLDER_ATTRIBUTES = {
      FILE_ATTRIBUTE_READONLY: 0x1,
      FILE_ATTRIBUTE_HIDDEN: 0x2,
      FILE_ATTRIBUTE_NORMAL: 0x80,
      FOLDER_ATTRIBUTE_READONLY: 0x1,
    };

    this.watcher = Watcher.Instance;

    this.syncRootPath = syncRootPath;
    this.createSyncRootFolder();

    const pathElements = this.syncRootPath.split("\\\\");
    pathElements.pop();
    const parentPath = pathElements.join("\\\\");
    this.addLoggerPath(loggerPath ?? parentPath);
  }

  addLoggerPath(loggerPath: string) {
    console.log("loggerPath: ", loggerPath);
    addon.addLoggerPath(loggerPath);
  }

  getPlaceholderState(pathRelative: string): Status {
    return addon.getPlaceholderState(this.syncRootPath + pathRelative);
  }

  getPlaceholderWithStatePending(): any {
    return addon.getPlaceholderWithStatePending(this.syncRootPath);
  }

  getInputSyncCallbacks(): InputSyncCallbacks {
    if (!this.callbacks) {
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
    if (!this.callbacks) {
      throw new Error("Callbacks are not defined");
    }

    const extraCallbackKeys: (keyof ExtraCallbacks)[] = [
      "notifyFileAddedCallback",
      "notifyMessageCallback",
    ];

    const result: ExtraCallbacks = {};
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
    return addon.connectSyncRoot(this.syncRootPath, this.getInputSyncCallbacks());
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
    const creationTimeStr = this.convertToWindowsTime(creationTime).toString();
    const lastWriteTimeStr = this.convertToWindowsTime(lastWriteTime).toString();
    const lastAccessTimeStr = this.convertToWindowsTime(lastAccessTime).toString();

    return addon.createPlaceholderFile(
      fileName,
      fileId,
      fileSize,
      fileAttributes,
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
    parentPath: string
  ) {
    const creationTimeStr = this.convertToWindowsTime(creationTime).toString();
    const lastWriteTimeStr = this.convertToWindowsTime(lastWriteTime).toString();
    return addon.createEntry(
      itemName,
      itemId,
      isDirectory,
      itemSize,
      fileAttributes,
      creationTimeStr,
      lastWriteTimeStr,
      lastAccessTime,
      parentPath
    );
  }

  private isValidFolderPath(p: string): boolean {
    return p.startsWith("/") && p.endsWith("/") && !p.includes(".");
  }

  private isValidFilePath(p: string): boolean {
    return p.includes(".");
  }

  async registerSyncRoot(
    providerName: string,
    providerVersion: string,
    providerId: string,
    callbacks: Callbacks,
    logoPath: string
  ): Promise<any> {
    this.callbacks = callbacks;
    return addon.registerSyncRoot(
      this.syncRootPath,
      providerName,
      providerVersion,
      providerId,
      logoPath
    );
  }

  static unregisterSyncRoot(syncRootPath: string): any {
    return addon.unregisterSyncRoot(syncRootPath);
  }

  private test(): void {
    console.log("Test");
  }

  watchAndWait(
    targetPath: string,
    queueManager: IQueueManager,
    loggerPath: string
  ): void {
    if (!this.callbacks) {
      throw new Error("Callbacks are not defined");
    }

    this.watcher.queueManager = queueManager;
    this.watcher.logPath = loggerPath;
    this.watcher.syncRootPath = targetPath;
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
      CfAddItem: this.test.bind(this),
      CfDehydrate: this.test.bind(this),
      CfHydrate: this.test.bind(this),
      CfNotifyMessage: this.test.bind(this),
      CfUpdateItem: this.test.bind(this),
      CfGetPlaceHolderAttributes: addon.getPlaceholderAttribute,
      // Ajustamos CfUpdateSyncStatus para que cumpla la firma requerida
      CfUpdateSyncStatus: (p: string, sync: boolean, isDirectory?: boolean): void => {
        const dirFlag = isDirectory ?? false;
        addon.updateSyncStatus(p, sync, dirFlag).catch((error: any) =>
          console.error(`Error en CfUpdateSyncStatus: ${error}`)
        );
      },
      CfGetPlaceHolderIdentity: addon.getFileIdentity,
      CfGetPlaceHolderState: addon.getPlaceholderState,
      CfConverToPlaceholder: addon.convertToPlaceholder,
    };

    this.watcher.watchAndWait();
  }

  private ensureDirectoriesForPath(splitPath: string[]): string {
    let currentPath = path.resolve(this.syncRootPath);
    for (let i = 0; i < splitPath.length - 1; i++) {
      const dir = splitPath[i];
      currentPath = path.join(currentPath, dir);

      if (fs.existsSync(path.dirname(currentPath))) {
        try {
          if (!fs.existsSync(currentPath)) {
            this.createPlaceholderDirectory(
              dir,
              `dir-${dir}-${Date.now()}`,
              true,
              0,
              this.PLACEHOLDER_ATTRIBUTES.FILE_ATTRIBUTE_NORMAL,
              Date.now(),
              Date.now(),
              Date.now(),
              path.dirname(currentPath)
            );
          }
        } catch (error: any) {
          console.error(`Error while creating directory: ${error.message}`);
        }
      }
    }
    return currentPath;
  }

  createFileByPath(
    relativePath: string,
    itemId: string,
    size: number = 0,
    creationTime: number = Date.now(),
    lastWriteTime: number = Date.now()
  ): void {
    const splitPath = relativePath.split("/").filter(Boolean);
    const currentPath = this.ensureDirectoriesForPath(splitPath);
    const fileName = path.basename(relativePath);
    try {
      this.createPlaceholderFile(
        fileName,
        itemId,
        size,
        this.PLACEHOLDER_ATTRIBUTES.FILE_ATTRIBUTE_NORMAL,
        creationTime,
        lastWriteTime,
        Date.now(),
        currentPath
      );
    } catch (error: any) {
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
    const splitPath = relativePath.split("/").filter(Boolean);
    let currentPath = path.resolve(this.syncRootPath);
    for (let i = 0; i < splitPath.length; i++) {
      const dir = splitPath[i];
      const last = i === splitPath.length - 1;
      if (last && fs.existsSync(currentPath)) {
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
        } catch (error: any) {
          console.error(`Error while creating directory: ${error.message}`);
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
    if (this.isValidFolderPath(relativePath)) {
      const splitPath = relativePath.split("/").filter(Boolean);
      let currentPath = path.resolve(this.syncRootPath);
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
          } catch (error: any) {
            console.error(`Error while creating directory: ${error.message}`);
          }
        }
        currentPath = path.join(currentPath, dir);
      }
    } else if (this.isValidFilePath(relativePath)) {
      const splitPath = relativePath.split("/").filter(Boolean);
      const currentPath = this.ensureDirectoriesForPath(splitPath);
      const fileName = path.basename(relativePath);
      try {
        this.createPlaceholderFile(
          fileName,
          itemId,
          size,
          this.PLACEHOLDER_ATTRIBUTES.FILE_ATTRIBUTE_NORMAL,
          creationTime,
          lastWriteTime,
          Date.now(),
          currentPath
        );
      } catch (error: any) {
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
    let fullPath = itemPath;
    if (!itemPath.includes(this.syncRootPath)) {
      fullPath = path.join(this.syncRootPath, itemPath);
    }
    return addon.updateSyncStatus(fullPath, sync, isDirectory);
  }

  convertToPlaceholder(itemPath: string, id: string): boolean {
    return addon.convertToPlaceholder(itemPath, id);
  }

  updateFileIdentity(itemPath: string, id: string, isDirectory: boolean): void {
    let fullPath = itemPath;
    if (!itemPath.includes(this.syncRootPath)) {
      fullPath = path.join(this.syncRootPath, itemPath);
    }
    addon.updateFileIdentity(fullPath, id, isDirectory);
  }

  closeDownloadMutex(): void {
    addon.closeMutex();
  }

  async dehydrateFile(itemPath: string): Promise<void> {
    return addon.dehydrateFile(itemPath);
  }

  async hydrateFile(itemPath: string): Promise<void> {
    return addon.hydrateFile(itemPath);
  }

  async getPlaceholderAttribute(itemPath: string): Promise<any> {
    return addon.getPlaceholderAttribute(itemPath);
  }
}

export default VirtualDrive;
