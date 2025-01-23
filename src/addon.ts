import { InputSyncCallbacks } from "./types/callbacks.type";

export const addon: Addon = require("../addon.node");

export type Addon = {
  addLoggerPath(path: string): any;
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
  closeMutex(): any;
  hydrateFile(path: string): Promise<void>;
  dehydrateFile(path: string): Promise<void>;
  connectSyncRoot(path: string, callbacks: InputSyncCallbacks): any;
  convertToPlaceholder(path: string, id: string): boolean;
  deleteFileSyncRoot(path: string): any;
  getFileIdentity(path: string): any;
  disconnectSyncRoot(path: string): any;
  getInputSyncCallbacks(): any;
  getItemsIds(path: string): any;
  getPlaceholderAttribute(path: string): Promise<any>;
  getPlaceholderState(path: string): any;
  getPlaceholderWithStatePending(path: string): string[];
  registerSyncRoot(
    providerName: string,
    providerVersion: string,
    providerId: string,
    callbacks: any,
    logoPath: string
  ): any;
  registerSyncRootWindowsStorageProvider(
    path: string,
    providerName: string,
    providerVersion: string,
    providerId: string
  ): any;
  unregisterSyncRoot(path: string): any;
  watchAndWait(path: string): any;
  updateSyncStatus(path: string, sync: boolean, isDirectory: boolean): Promise<void>;
  updateFileIdentity(itemPath: string, id: string, isDirectory: boolean): any;
};
