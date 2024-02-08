declare module "addon" {
  export interface Addon {
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
    convertToPlaceholder(path: string): boolean;
  }

  const addon: Addon;
  export default addon;
}
