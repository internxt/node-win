import path, { join } from "path";
import fs from "fs";
import { Watcher } from "./watcher/watcher";
import { Callbacks } from "./types/callbacks.type";
import { Status } from "./types/placeholder.type";
import { IQueueManager } from "./queue/queueManager";

import { addon } from "./addon";

interface ItemInfo {
  path: string;
  fileIdentity: string;
  isPlaceholder: boolean;
}

class VirtualDrive {
  PLACEHOLDER_ATTRIBUTES = {
    FILE_ATTRIBUTE_READONLY: 0x1,
    FILE_ATTRIBUTE_HIDDEN: 0x2,
    FOLDER_ATTRIBUTE_READONLY: 0x1,
    FILE_ATTRIBUTE_NORMAL: 0x1,
  };

  syncRootPath: string;
  callbacks?: Callbacks;

  private watcher: Watcher;

  constructor(syncRootPath: string, loggerPath: string) {
    this.watcher = Watcher.Instance;
    this.syncRootPath = syncRootPath;
    this.createSyncRootFolder();
    this.addLoggerPath(loggerPath);
  }

  private fixPath(path: string) {
    if (path.includes(this.syncRootPath)) {
      return path;
    } else {
      return join(this.syncRootPath, path);
    }
  }

  addLoggerPath(loggerPath: string) {
    addon.addLoggerPath(loggerPath);
  }

  getPlaceholderState(path: string): Status {
    return addon.getPlaceholderState(this.fixPath(path));
  }

  getPlaceholderWithStatePending(): any {
    return addon.getPlaceholderWithStatePending(this.syncRootPath);
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
    return addon.getItemsIds(this.syncRootPath);
  }

  async getFileIdentity(path: string): Promise<string> {
    return addon.getFileIdentity(this.fixPath(path));
  }

  async deleteFileSyncRoot(relativePath: string): Promise<void> {
    const fullPath = join(this.syncRootPath, relativePath);
    return addon.deleteFileSyncRoot(fullPath);
  }

  async connectSyncRoot(): Promise<any> {
    if (this.callbacks === undefined) {
      throw new Error("Callbacks are not defined");
    }

    return await addon.connectSyncRoot(this.syncRootPath, this.callbacks);
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
    const lastWriteTimeStr =
      this.convertToWindowsTime(lastWriteTime).toString();
    const lastAccessTimeStr =
      this.convertToWindowsTime(lastAccessTime).toString();

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
    return addon.unregisterSyncRoot(syncRootPath);
  }

  watchAndWait(
    path: string,
    queueManager: IQueueManager,
    loggerPath: string
  ): void {
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
      CfAddItem: console.log,
      CfDehydrate: console.log,
      CfHydrate: console.log,
      CfNotifyMessage: console.log,
      CfUpdateItem: console.log,
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

    // Only creates the last folder
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

  disconnectSyncRoot(): any {
    return addon.disconnectSyncRoot(this.syncRootPath);
  }

  async updateSyncStatus(
    itemPath: string,
    isDirectory: boolean,
    sync: boolean = true
  ): Promise<void> {
    return await addon.updateSyncStatus(
      this.fixPath(itemPath),
      sync,
      isDirectory
    );
  }

  convertToPlaceholder(itemPath: string, id: string): boolean {
    return addon.convertToPlaceholder(itemPath, id);
  }

  updateFileIdentity(itemPath: string, id: string, isDirectory: boolean): void {
    addon.updateFileIdentity(this.fixPath(itemPath), id, isDirectory);
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
}

export default VirtualDrive;
