import path from 'path';
import fs from 'fs';
import { deleteAllSubfolders } from './utils';
import { Worker } from 'worker_threads';

const addonPath = path.join(__dirname, '../../build/Release/addon.node');
const addon = require(addonPath);

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

type ExtraCallbacks = {
    notifyFileAddedCallback?: NapiCallbackFunction;
}

type Callbacks = InputSyncCallbacks & ExtraCallbacks;
class VirtualDrive {
    PLACEHOLDER_ATTRIBUTES: { [key: string]: number };
    syncRootPath: string;
    callbacks?: Callbacks;

    constructor(syncRootPath: string) {

        this.PLACEHOLDER_ATTRIBUTES = {
            FILE_ATTRIBUTE_READONLY: 0x1,
            FILE_ATTRIBUTE_HIDDEN: 0x2,
            FOLDER_ATTRIBUTE_READONLY: 0x1,
        };

        this.syncRootPath = syncRootPath;
    }

    getInputSyncCallbacks(): InputSyncCallbacks {
        if (this.callbacks === undefined) {
            throw new Error('Callbacks are not defined');
        }
        
        const inputSyncCallbackKeys: (keyof InputSyncCallbacks)[] = [
            'fetchDataCallback',
            'validateDataCallback',
            'cancelFetchDataCallback',
            'fetchPlaceholdersCallback',
            'cancelFetchPlaceholdersCallback',
            'notifyFileOpenCompletionCallback',
            'notifyFileCloseCompletionCallback',
            'notifyDehydrateCallback',
            'notifyDehydrateCompletionCallback',
            'notifyDeleteCallback',
            'notifyDeleteCompletionCallback',
            'notifyRenameCallback',
            'notifyRenameCompletionCallback',
            'noneCallback'
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
        const extraCallbackKeys: (keyof ExtraCallbacks)[] = [
            'notifyFileAddedCallback'
        ];

        const result: ExtraCallbacks = {};
        if (this.callbacks === undefined) {
            throw new Error('Callbacks are not defined');
        }
        
        for (const key of extraCallbackKeys) {
            if (this.callbacks[key] !== undefined) {
                result[key] = this.callbacks[key];
            }
        }

        return result;
    }

    convertToWindowsTime(jsTime: number): bigint {
        return BigInt(jsTime) * 10000n + 116444736000000000n;
    }

    async connectSyncRoot(): Promise<any> {
        if (this.callbacks === undefined) {
            throw new Error('Callbacks are not defined');
        }
        return await addon.connectSyncRoot(this.syncRootPath, this.getInputSyncCallbacks());
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
        return addon.createEntry(
            itemName,
            itemId,
            isDirectory,
            itemSize,
            fileAttributes,
            creationTime,
            lastWriteTime,
            lastAccessTime,
            path
        )
    }

    async registerSyncRoot(providerName: string, providerVersion: string, providerId: string, callbacks: Callbacks): Promise<any> {
        this.callbacks = callbacks;
        return await addon.registerSyncRoot(this.syncRootPath, providerName, providerVersion, providerId);
    }

    unregisterSyncRoot(): any {
        const result = addon.unregisterSyncRoot(this.syncRootPath);
        deleteAllSubfolders(this.syncRootPath);
        return result;
    }

    static unregisterSyncRoot(syncRootPath: string): any {
        const result = addon.unregisterSyncRoot(syncRootPath);
        deleteAllSubfolders(syncRootPath);
        return result;
    }

    watchAndWait(path: string): void {
        if (this.callbacks === undefined) {
            throw new Error('Callbacks are not defined');
        }
        
        addon.watchAndWait(path, this.getExtraCallbacks());
    }
    
    createItemByPath(relativePath: string, itemId: string, size: number = 0) {
        const fullPath = path.join(this.syncRootPath, relativePath);
    
        if (relativePath.endsWith('/')) { // is a folder
            const splitPath = relativePath.split('/').filter(p => p);
            let currentPath = this.syncRootPath;
    
            for (const dir of splitPath) {
                currentPath = path.join(currentPath, dir);
    
                if (!fs.existsSync(currentPath)) {
                    try {
                        this.createPlaceholderDirectory(
                            dir,
                            itemId,
                            true,
                            size,
                            this.PLACEHOLDER_ATTRIBUTES.FOLDER_ATTRIBUTE_READONLY,
                            Date.now(),
                            Date.now(),
                            Date.now(),
                            currentPath
                        );
                    } catch (error) {
                        //@ts-ignore
                        console.error(`Error while creating directory: ${error.message}`);
                    }
                }
            }
        } else { // is a file
            const directoryPath = path.dirname(fullPath);
    
            if (!fs.existsSync(directoryPath)) {
                fs.mkdirSync(directoryPath, { recursive: true });
            }
    
            try {
                this.createPlaceholderFile(
                    path.basename(fullPath),
                    itemId,
                    size,
                    this.PLACEHOLDER_ATTRIBUTES.FILE_ATTRIBUTE_READONLY,
                    Date.now(),
                    Date.now(),
                    Date.now(),
                    directoryPath
                );
            } catch (error) {
                //@ts-ignore
                console.error(`Error al crear placeholder: ${error.message}`);
            }
        }
    }
    
}

export default VirtualDrive;
