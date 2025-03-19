import { logger } from "examples/drive";
import fs from "fs";
import path, { join, win32 } from "path";
import winston from "winston";

import { Addon } from "./addon-wrapper";
import { createLogger } from "./logger";
import { IQueueManager } from "./queue/queueManager";
import { Callbacks } from "./types/callbacks.type";
import { Watcher } from "./watcher/watcher";

const PLACEHOLDER_ATTRIBUTES = {
  FILE_ATTRIBUTE_READONLY: 0x1,
  FILE_ATTRIBUTE_HIDDEN: 0x2,
  FOLDER_ATTRIBUTE_READONLY: 0x1,
  FILE_ATTRIBUTE_NORMAL: 0x1,
};

class VirtualDrive {
  syncRootPath: string;
  providerId: string;
  callbacks?: Callbacks;
  watcher = new Watcher();
  logger: winston.Logger;

  addon: Addon;

  constructor(syncRootPath: string, providerId: string, loggerPath: string) {
    this.addon = new Addon();
    this.syncRootPath = this.convertToWindowsPath(syncRootPath);
    loggerPath = this.convertToWindowsPath(loggerPath);
    this.providerId = providerId;

    this.addon.syncRootPath = this.syncRootPath;

    this.createSyncRootFolder();
    this.addLoggerPath(loggerPath);
    this.logger = createLogger(loggerPath);
  }

  convertToWindowsTime(jsTime: number) {
    return BigInt(jsTime) * 10000n + 116444736000000000n;
  }

  convertToWindowsPath(path: string) {
    return path.replaceAll("/", win32.sep);
  }

  fixPath(path: string) {
    path = this.convertToWindowsPath(path);
    if (path.includes(this.syncRootPath)) {
      return path;
    } else {
      return join(this.syncRootPath, path);
    }
  }

  addLoggerPath(logPath: string) {
    this.addon.addLogger({ logPath });
  }

  getPlaceholderState(path: string) {
    return this.addon.getPlaceholderState({ path: this.fixPath(path) });
  }

  getPlaceholderWithStatePending() {
    return this.addon.getPlaceholderWithStatePending();
  }

  createSyncRootFolder() {
    if (!fs.existsSync(this.syncRootPath)) {
      fs.mkdirSync(this.syncRootPath, { recursive: true });
    }
  }

  getFileIdentity(relativePath: string) {
    return this.addon.getFileIdentity({ path: this.fixPath(relativePath) });
  }

  async deleteFileSyncRoot(relativePath: string) {
    return this.addon.deleteFileSyncRoot({ path: this.fixPath(relativePath) });
  }

  connectSyncRoot() {
    if (this.callbacks === undefined) {
      throw new Error("Callbacks are not defined");
    }

    const connectionKey = this.addon.connectSyncRoot({ callbacks: this.callbacks });

    this.logger.debug({ fn: "connectSyncRoot", connectionKey });
    return connectionKey;
  }

  disconnectSyncRoot() {
    this.addon.disconnectSyncRoot({ syncRootPath: this.syncRootPath });
  }

  createPlaceholderFile(
    fileName: string,
    fileId: string,
    fileSize: number,
    fileAttributes: number,
    creationTime: number,
    lastWriteTime: number,
    lastAccessTime: number,
    basePath: string,
  ): any {
    const creationTimeStr = this.convertToWindowsTime(creationTime).toString();
    const lastWriteTimeStr = this.convertToWindowsTime(lastWriteTime).toString();
    const lastAccessTimeStr = this.convertToWindowsTime(lastAccessTime).toString();

    return this.addon.createPlaceholderFile({
      fileName,
      fileId,
      fileSize,
      fileAttributes,
      creationTime: creationTimeStr,
      lastWriteTime: lastWriteTimeStr,
      lastAccessTime: lastAccessTimeStr,
      basePath,
    });
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
    path: string,
  ) {
    const creationTimeStr = this.convertToWindowsTime(creationTime).toString();
    const lastWriteTimeStr = this.convertToWindowsTime(lastWriteTime).toString();
    const lastAccessTimeStr = this.convertToWindowsTime(lastAccessTime).toString();

    return this.addon.createPlaceholderDirectory({
      itemName,
      itemId,
      isDirectory,
      itemSize,
      fileAttributes,
      creationTime: creationTimeStr,
      lastWriteTime: lastWriteTimeStr,
      lastAccessTime: lastAccessTimeStr,
      path,
    });
  }

  async registerSyncRoot(providerName: string, providerVersion: string, callbacks: Callbacks, logoPath: string): Promise<any> {
    this.callbacks = callbacks;
    console.log("Registering sync root: ", this.syncRootPath);
    return this.addon.registerSyncRoot({
      providerName,
      providerVersion,
      providerId: this.providerId,
      logoPath,
    });
  }

  unregisterSyncRoot() {
    return this.addon.unregisterSyncRoot({ providerId: this.providerId });
  }

  unRegisterSyncRootByProviderId(providerId: string) {
    return this.addon.unregisterSyncRoot({ providerId });
  }

  watchAndWait(path: string, queueManager: IQueueManager, loggerPath: string): void {
    this.watcher.addon = this.addon;
    this.watcher.queueManager = queueManager;
    this.watcher.logger = this.logger;
    this.watcher.syncRootPath = this.syncRootPath;
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

    this.watcher.watchAndWait();
  }

  createFileByPath(
    relativePath: string,
    itemId: string,
    size: number = 0,
    creationTime: number = Date.now(),
    lastWriteTime: number = Date.now(),
  ) {
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
        PLACEHOLDER_ATTRIBUTES.FILE_ATTRIBUTE_NORMAL,
        creationTime,
        lastWriteTime,
        Date.now(),
        currentPath,
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
    lastWriteTime: number = Date.now(),
  ) {
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
            PLACEHOLDER_ATTRIBUTES.FILE_ATTRIBUTE_NORMAL,
            creationTime,
            lastWriteTime,
            Date.now(),
            currentPath,
          );
        }
      }
      currentPath = path.join(currentPath, dir);
    }
  }

  updateSyncStatus(itemPath: string, isDirectory: boolean, sync: boolean = true) {
    return this.addon.updateSyncStatus({ path: this.fixPath(itemPath), isDirectory, sync });
  }

  convertToPlaceholder(itemPath: string, id: string) {
    return this.addon.convertToPlaceholder({ path: this.fixPath(itemPath), id });
  }
  updateFileIdentity(itemPath: string, id: string, isDirectory: boolean) {
    return this.addon.updateFileIdentity({ path: this.fixPath(itemPath), id, isDirectory });
  }

  dehydrateFile(itemPath: string) {
    return this.addon.dehydrateFile({ path: this.fixPath(itemPath) });
  }

  hydrateFile(itemPath: string) {
    return this.addon.hydrateFile({ path: this.fixPath(itemPath) });
  }
}

export default VirtualDrive;
