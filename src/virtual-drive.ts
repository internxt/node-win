import path, { join, win32 } from "path";
import fs from "fs";
import { Watcher } from "./watcher/watcher";
import { ExtraCallbacks, InputSyncCallbacks } from "./types/callbacks.type";
import { IQueueManager } from "./queue/queueManager";

import { createLogger } from "./logger";
import { Addon } from "./addon-wrapper";

const addon = new Addon();

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
      FOLDER_ATTRIBUTE_READONLY: 0x1,
      FILE_ATTRIBUTE_NORMAL: 0x1,
    };

    this.watcher = new Watcher();
    addon.syncRootPath = syncRootPath;

    this.syncRootPath = syncRootPath;
    this.createSyncRootFolder();

    let pathElements = this.syncRootPath.split("\\\\");
    pathElements.pop();
    let parentPath = pathElements.join("\\\\");

    this.addLoggerPath(loggerPath ?? parentPath);
  }

  addLoggerPath(logPath: string) {
    addon.addLogger({ logPath });
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

  getPlaceholderState(path: string) {
    return addon.getPlaceholderState({ path: this.fixPath(path) });
  }

  getPlaceholderWithStatePending(): any {
    return addon.getPlaceholderWithStatePending();
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

  createSyncRootFolder() {
    if (!fs.existsSync(this.syncRootPath)) {
      fs.mkdirSync(this.syncRootPath, { recursive: true });
    }
  }

  convertToWindowsTime(jsTime: number): bigint {
    return BigInt(jsTime) * 10000n + 116444736000000000n;
  }

  getFileIdentity(relativePath: string) {
    return addon.getFileIdentity({ path: this.fixPath(relativePath) });
  }

  async deleteFileSyncRoot(relativePath: string): Promise<void> {
    return addon.deleteFileSyncRoot({ path: this.fixPath(relativePath) });
  }

  async connectSyncRoot(): Promise<any> {
    return addon.connectSyncRoot({ callbacks: this.getInputSyncCallbacks() });
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

    return addon.createPlaceholderFile({
      fileName,
      fileId,
      fileSize,
      fileAttributes,
      creationTime: creationTimeStr,
      lastWriteTime: lastWriteTimeStr,
      lastAccessTime: lastAccessTimeStr,
      basePath
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
    path: string
  ) {
    const creationTimeStr = this.convertToWindowsTime(creationTime).toString();
    const lastWriteTimeStr = this.convertToWindowsTime(lastWriteTime).toString();
    const lastAccessTimeStr = this.convertToWindowsTime(lastAccessTime).toString();
    
    return addon.createPlaceholderDirectory({
      itemName,
      itemId,
      isDirectory,
      itemSize,
      fileAttributes,
      creationTime: creationTimeStr,
      lastWriteTime: lastWriteTimeStr,
      lastAccessTime: lastAccessTimeStr,
      path
    });
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
    return addon.registerSyncRoot({
      providerName,
      providerVersion,
      providerId,
      logoPath
    });
  }

  static unregisterSyncRoot(syncRootPath: string): any {
    const result = addon.unregisterSyncRoot({ syncRootPath });
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
    if (this.callbacks === undefined) {
      throw new Error("Callbacks are not defined");
    }

    this.watcher.queueManager = queueManager;

    this.watcher.logger = createLogger(loggerPath);

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

    this.watcher.addon = addon;

    this.watcher.watchAndWait();
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

  disconnectSyncRoot() {
    return addon.disconnectSyncRoot();
  }

  async updateSyncStatus(
    itemPath: string,
    isDirectory: boolean,
    sync: boolean = true
  ) {
    return await addon.updateSyncStatus({ path: this.fixPath(itemPath), isDirectory, sync });
  }

  convertToPlaceholder(itemPath: string, id: string): boolean {
    return addon.convertToPlaceholder({ path: this.fixPath(itemPath), id });
  }
  updateFileIdentity(itemPath: string, id: string, isDirectory: boolean): void {
    addon.updateFileIdentity({ path: this.fixPath(itemPath), id, isDirectory });
  }

  async dehydrateFile(itemPath: string) {
    return addon.dehydrateFile({ path: this.fixPath(itemPath) });
  }

  async hydrateFile(itemPath: string): Promise<void> {
    return addon.hydrateFile({ path: this.fixPath(itemPath) });
  }
}

export default VirtualDrive;
