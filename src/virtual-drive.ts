import fs from "fs";
import path, { join, win32 } from "path";
import { Logger } from "winston";

import { addon } from "./addon";
import { addonZod } from "./addon/addon-zod";
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
  logger: Logger;
  callbacks!: Callbacks;

  private watcher = new Watcher();

  constructor(syncRootPath: string, loggerPath: string) {
    this.syncRootPath = this.convertToWindowsPath(syncRootPath);
    loggerPath = this.convertToWindowsPath(loggerPath);

    this.logger = createLogger(loggerPath);
    this.logger.info({ fn: "VirtualDrive.constructor", syncRootPath: this.syncRootPath, loggerPath });

    this.createSyncRootFolder();
    this.addLoggerPath(loggerPath);
  }

  private createSyncRootFolder() {
    if (!fs.existsSync(this.syncRootPath)) {
      fs.mkdirSync(this.syncRootPath, { recursive: true });
    }
  }

  private convertToWindowsTime(jsTime: number) {
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

  parseAddonZod<T>(fn: keyof typeof addonZod, data: T) {
    const schema = addonZod[fn];
    const result = schema.safeParse(data);
    if (result.error) this.logger.error(fn, result.error);
    return data;
  }

  addLoggerPath(loggerPath: string) {
    const result = addon.addLoggerPath(loggerPath);
    return this.parseAddonZod("addLoggerPath", result);
  }

  getPlaceholderState(path: string) {
    const result = addon.getPlaceholderState(this.fixPath(path));
    return this.parseAddonZod("getPlaceholderState", result);
  }

  getPlaceholderWithStatePending() {
    const result = addon.getPlaceholderWithStatePending(this.syncRootPath);
    return this.parseAddonZod("getPlaceholderWithStatePending", result);
  }

  getFileIdentity(path: string) {
    const result = addon.getFileIdentity(this.fixPath(path));
    return this.parseAddonZod("getFileIdentity", result);
  }

  async deleteFileSyncRoot(path: string) {
    return addon.deleteFileSyncRoot(this.fixPath(path));
  }

  connectSyncRoot() {
    const result = addon.connectSyncRoot(this.syncRootPath, {
      ...this.callbacks,
      fetchDataCallback: (...props) => {
        const id = props[0];
        this.watcher.fileInDevice.add(id);
        this.callbacks.fetchDataCallback(...props);
      }
    });
    return this.parseAddonZod("connectSyncRoot", result);
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

    return addon.createPlaceholderFile(
      fileName,
      fileId,
      fileSize,
      fileAttributes,
      creationTimeStr,
      lastWriteTimeStr,
      lastAccessTimeStr,
      basePath,
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
    path: string,
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
      path,
    );
  }

  registerSyncRoot(providerName: string, providerVersion: string, providerId: string, callbacks: Callbacks, logoPath: string) {
    this.callbacks = callbacks;
    const result = addon.registerSyncRoot(this.syncRootPath, providerName, providerVersion, providerId, logoPath);
    return this.parseAddonZod("registerSyncRoot", result);
  }

  static unregisterSyncRoot(syncRootPath: string) {
    const result = addon.unregisterSyncRoot(syncRootPath);
    // return this.parseAddonZod("unregisterSyncRoot", result);
    return result;
  }

  watchAndWait(path: string, queueManager: QueueManager, loggerPath: string) {
    if (this.callbacks === undefined) {
      throw new Error("Callbacks are not defined");
    }

    const options = {
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

    const virtualDriveFn = {
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

    queueManager.logger = this.logger;

    this.watcher.init(queueManager, this.syncRootPath, options, this.logger, virtualDriveFn);
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

  disconnectSyncRoot() {
    addon.disconnectSyncRoot(this.syncRootPath);
  }

  /**
   * @deprecated
   */
  updateSyncStatus(path: string, isDirectory: boolean, sync: boolean = true) {
    const result = addon.updateSyncStatus(this.fixPath(path), sync, isDirectory);
    return this.parseAddonZod("updateSyncStatus", result);
  }

  convertToPlaceholder(path: string, id: string) {
    const result = addon.convertToPlaceholder(this.fixPath(path), id);
    return this.parseAddonZod("convertToPlaceholder", result);
  }

  updateFileIdentity(path: string, id: string, isDirectory: boolean) {
    addon.updateFileIdentity(this.fixPath(path), id, isDirectory);
  }

  dehydrateFile(path: string) {
    const result = addon.dehydrateFile(this.fixPath(path));
    return this.parseAddonZod("dehydrateFile", result);
  }

  async hydrateFile(path: string) {
    const result = await addon.hydrateFile(this.fixPath(path));
    return this.parseAddonZod("hydrateFile", result);
  }

  getPlaceholderAttribute(path: string) {
    const result = addon.getPlaceholderAttribute(this.fixPath(path));
    return this.parseAddonZod("getPlaceholderAttribute", result);
  }
}

export default VirtualDrive;
