const addon = require('../../build/Release/addon.node');

interface Addon {
    connectSyncRoot(path: string): any;
    createPlaceholderFile(fileName: string, fileId: string, fileSize: number, combinedAttributes: number, creationTime: string, lastWriteTime: string, lastAccessTime: string, path: string): any;
    registerSyncRootWindowsStorageProvider(path: string, providerName: string, providerVersion: string, providerId: string): any;
    unregisterSyncRoot(path: string): any;
    watchAndWait(path: string): any;
}

type NapiCallbackFunction = (...args: any[]) => any;

type InputSyncCallbacks = {
    fetchDataCallback?: NapiCallbackFunction;
    validateDataCallback?: NapiCallbackFunction;
    cancelFetchDataCallback?: NapiCallbackFunction;
    fetchPlaceholdersCallback?: NapiCallbackFunction;
    cancelFetchPlaceholdersCallback?: NapiCallbackFunction;
    notifyFileOpenCompletionCallback?: NapiCallbackFunction;
    notifyFileCloseCompletionCallback?: NapiCallbackFunction;
    notifyDehydrateCallback?: NapiCallbackFunction;
    notifyDehydrateCompletionCallback?: NapiCallbackFunction;
    notifyDeleteCallback?: NapiCallbackFunction;
    notifyDeleteCompletionCallback?: NapiCallbackFunction;
    notifyRenameCallback?: NapiCallbackFunction;
    notifyRenameCompletionCallback?: NapiCallbackFunction;
    noneCallback?: NapiCallbackFunction;
}
class VirtualDrive {
    PLACEHOLDER_ATTRIBUTES: { [key: string]: number };

    constructor() {

        this.PLACEHOLDER_ATTRIBUTES = {
            FILE_ATTRIBUTE_READONLY: 0x1,
            FILE_ATTRIBUTE_HIDDEN: 0x2,
        };
    }

    convertToWindowsTime(jsTime: number): bigint {
        return BigInt(jsTime) * 10000n + 116444736000000000n;
    }

    async connectSyncRoot(path: string, callbacks: InputSyncCallbacks): Promise<any> {
        return await addon.connectSyncRoot(path, callbacks);
    }

    createPlaceholderFile(fileName: string, fileId: string, fileSize: number, fileAttributes: number, creationTime: number, lastWriteTime: number, lastAccessTime: number, path: string): any {
        const combinedAttributes = fileAttributes;
        const creationTimeStr = this.convertToWindowsTime(creationTime).toString();
        const lastWriteTimeStr = this.convertToWindowsTime(lastWriteTime).toString();
        const lastAccessTimeStr = this.convertToWindowsTime(lastAccessTime).toString();

        return addon.createPlaceholderFile(
            fileName,
            fileId,
            fileSize,
            combinedAttributes,
            creationTimeStr,
            lastWriteTimeStr,
            lastAccessTimeStr,
            path
        );
    }

    async registerSyncRoot(path: string, providerName: string, providerVersion: string, providerId: string): Promise<any> {
        return await addon.registerSyncRoot(path, providerName, providerVersion, providerId);
    }

    unregisterSyncRoot(path: string): any {
        return addon.unregisterSyncRoot(path);
    }

    watchAndWait(path: string): any {
        return addon.watchAndWait(path);
    }
}

export default VirtualDrive;
