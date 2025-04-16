import fs from "fs";
import path, { join, win32 } from "path";
import winston from "winston";

import { Addon, DependencyInjectionAddonProvider } from "./addon-wrapper";
import { createLogger } from "./logger";
import { QueueManager } from "./queue/queue-manager";
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
    this.addon = DependencyInjectionAddonProvider.get();
    this.syncRootPath = this.convertToWindowsPath({ path: syncRootPath });
    loggerPath = this.convertToWindowsPath({ path: loggerPath });
    this.providerId = providerId;

    this.addon.syncRootPath = this.syncRootPath;

    this.createSyncRootFolder();
    this.addLoggerPath(loggerPath);
    this.logger = createLogger(loggerPath);
  }

  private convertToWindowsTime(jsTime: number) {
    return BigInt(jsTime) * 10000n + 116444736000000000n;
  }

  convertToWindowsPath({ path }: { path: string }) {
    return path.replaceAll("/", win32.sep);
  }

  fixPath(path: string) {
    path = this.convertToWindowsPath({ path });
    if (path.includes(this.syncRootPath)) {
      return path;
    } else {
      return join(this.syncRootPath, path);
    }
  }

  addLoggerPath(logPath: string) {
    this.addon.addLogger({ logPath });
  }

  getPlaceholderState({ path }: { path: string }) {
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

  getFileIdentity({ path }: { path: string }) {
    return this.addon.getFileIdentity({ path: this.fixPath(path) });
  }

  async deleteFileSyncRoot({ path }: { path: string }) {
    return this.addon.deleteFileSyncRoot({ path: this.fixPath(path) });
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

  private createPlaceholderFile({
    fileName,
    fileId,
    fileSize,
    fileAttributes,
    creationTime,
    lastWriteTime,
    lastAccessTime,
    basePath = this.syncRootPath,
  }: {
    fileName: string;
    fileId: string;
    fileSize: number;
    fileAttributes: number;
    creationTime: number;
    lastWriteTime: number;
    lastAccessTime: number;
    basePath?: string;
  }): any {
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

  private createPlaceholderDirectory({
    itemName,
    itemId,
    isDirectory,
    itemSize,
    folderAttributes,
    creationTime,
    lastWriteTime,
    lastAccessTime,
    path = this.syncRootPath,
  }: {
    itemName: string;
    itemId: string;
    isDirectory: boolean;
    itemSize: number;
    folderAttributes: number;
    creationTime: number;
    lastWriteTime: number;
    lastAccessTime: number;
    path?: string;
  }) {
    const creationTimeStr = this.convertToWindowsTime(creationTime).toString();
    const lastWriteTimeStr = this.convertToWindowsTime(lastWriteTime).toString();
    const lastAccessTimeStr = this.convertToWindowsTime(lastAccessTime).toString();

    return this.addon.createPlaceholderDirectory({
      itemName,
      itemId,
      isDirectory,
      itemSize,
      folderAttributes,
      creationTime: creationTimeStr,
      lastWriteTime: lastWriteTimeStr,
      lastAccessTime: lastAccessTimeStr,
      path,
    });
  }

  async registerSyncRoot({
    providerName,
    providerVersion,
    callbacks,
    logoPath,
  }: {
    providerName: string;
    providerVersion: string;
    callbacks: Callbacks;
    logoPath: string;
  }): Promise<any> {
    this.callbacks = callbacks;
    this.logger.debug({ msg: "Registering sync root", syncRootPath: this.syncRootPath });
    return this.addon.registerSyncRoot({
      providerName,
      providerVersion,
      providerId: this.providerId,
      logoPath,
    });
  }

  static getRegisteredSyncRoots() {
    return DependencyInjectionAddonProvider.get().getRegisteredSyncRoots();
  }

  unregisterSyncRoot() {
    return this.addon.unregisterSyncRoot({ providerId: this.providerId });
  }

  static unRegisterSyncRootByProviderId({ providerId }: { providerId: string }) {
    return DependencyInjectionAddonProvider.get().unregisterSyncRoot({ providerId });
  }

  watchAndWait({ queueManager }: { queueManager: QueueManager }): void {
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

  createFileByPath({
    relativePath,
    itemId,
    size = 0,
    creationTime = Date.now(),
    lastWriteTime = Date.now(),
  }: {
    relativePath: string;
    itemId: string;
    size?: number;
    creationTime?: number;
    lastWriteTime?: number;
  }) {
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
      this.createPlaceholderFile({
        fileName: path.basename(fullPath),
        fileId: itemId,
        fileSize: size,
        fileAttributes: PLACEHOLDER_ATTRIBUTES.FILE_ATTRIBUTE_NORMAL,
        creationTime,
        lastWriteTime,
        lastAccessTime: Date.now(),
        basePath: currentPath,
      });
    } catch (error) {
      //@ts-ignore
      console.error(`Error al crear placeholder: ${error.message}`);
    }
  }

  createFolderByPath({
    relativePath,
    itemId,
    size = 0,
    creationTime = Date.now(),
    lastWriteTime = Date.now(),
  }: {
    relativePath: string;
    itemId: string;
    size?: number;
    creationTime?: number;
    lastWriteTime?: number;
  }) {
    const splitPath = relativePath.split("/").filter((p) => p);
    const directoryPath = path.resolve(this.syncRootPath);
    let currentPath = directoryPath;
    // solo crear el ultimo directorio
    for (let i = 0; i < splitPath.length; i++) {
      const dir = splitPath[i];
      const last = i === splitPath.length - 1;
      if (last) {
        if (fs.existsSync(currentPath)) {
          this.createPlaceholderDirectory({
            itemName: dir,
            itemId,
            isDirectory: true,
            itemSize: size,
            folderAttributes: PLACEHOLDER_ATTRIBUTES.FOLDER_ATTRIBUTE_READONLY,
            creationTime,
            lastWriteTime: lastWriteTime,
            lastAccessTime: Date.now(),
            path: currentPath,
          });
        }
      }
      currentPath = path.join(currentPath, dir);
    }
  }

  updateSyncStatus({ itemPath, isDirectory, sync = true }: { itemPath: string; isDirectory: boolean; sync?: boolean }) {
    return this.addon.updateSyncStatus({ path: this.fixPath(itemPath), isDirectory, sync });
  }

  convertToPlaceholder({ itemPath, id }: { itemPath: string; id: string }) {
    return this.addon.convertToPlaceholder({ path: this.fixPath(itemPath), id });
  }

  updateFileIdentity({ itemPath, id, isDirectory }: { itemPath: string; id: string; isDirectory: boolean }) {
    return this.addon.updateFileIdentity({ path: this.fixPath(itemPath), id, isDirectory });
  }

  dehydrateFile({ itemPath }: { itemPath: string }) {
    return this.addon.dehydrateFile({ path: this.fixPath(itemPath) });
  }

  hydrateFile({ itemPath }: { itemPath: string }) {
    return this.addon.hydrateFile({ path: this.fixPath(itemPath) });
  }
}

export default VirtualDrive;
