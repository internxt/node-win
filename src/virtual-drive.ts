const addon = require('../../build/Release/addon.node');

interface Addon {
    connectSyncRoot(path: string): any;
    createPlaceholderFile(fileName: string, fileId: string, fileSize: number, combinedAttributes: number, creationTime: string, lastWriteTime: string, lastAccessTime: string, path: string): any;
    registerSyncRootWindowsStorageProvider(path: string, providerName: string, providerVersion: string, providerId: string): any;
    unregisterSyncRoot(path: string): any;
    watchAndWait(path: string): any;
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

    connectSyncRoot(path: string): any {
        return addon.connectSyncRoot(path);
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

    registerSyncRoot(path: string, providerName: string, providerVersion: string, providerId: string): any {
        return addon.registerSyncRoot(path, providerName, providerVersion, providerId);
    }

    unregisterSyncRoot(path: string): any {
        return addon.unregisterSyncRoot(path);
    }

    watchAndWait(path: string): any {
        return addon.watchAndWait(path);
    }
}

export default VirtualDrive;
