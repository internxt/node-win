const addon = require('../../build/Release/addon.node');
import path from 'path';
import fs from 'fs';
import { deleteAllSubfolders } from './utils';

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
    syncRootPath: string;

    constructor(syncRootPath: string) {

        this.PLACEHOLDER_ATTRIBUTES = {
            FILE_ATTRIBUTE_READONLY: 0x1,
            FILE_ATTRIBUTE_HIDDEN: 0x2,
            FOLDER_ATTRIBUTE_READONLY: 0x1,
        };

        this.syncRootPath = syncRootPath;
    }

    convertToWindowsTime(jsTime: number): bigint {
        return BigInt(jsTime) * 10000n + 116444736000000000n;
    }

    async connectSyncRoot(callbacks: InputSyncCallbacks): Promise<any> {
        return await addon.connectSyncRoot(this.syncRootPath, callbacks);
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

    async registerSyncRoot(providerName: string, providerVersion: string, providerId: string): Promise<any> {
        return await addon.registerSyncRoot(this.syncRootPath, providerName, providerVersion, providerId);
    }

    unregisterSyncRoot(): any {
        const result = addon.unregisterSyncRoot(this.syncRootPath);
        deleteAllSubfolders(this.syncRootPath);
        return result;
    }

    watchAndWait(path: string): any {
        return addon.watchAndWait(path);
    }
    
    createItemByPath(relativePath: string, fileId: string) {
        const fullPath = path.join(this.syncRootPath, relativePath);
        console.log(`fullDestPath: ${fullPath}`);
    
        if (relativePath.endsWith('/')) { // Es un directorio
            const splitPath = relativePath.split('/').filter(p => p);
            let currentPath = this.syncRootPath;
    
            for (const dir of splitPath) {
                currentPath = path.join(currentPath, dir);
    
                if (!fs.existsSync(currentPath)) {
                    try {
                        this.createPlaceholderDirectory(
                            dir,
                            fileId,
                            true,
                            0,
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
        } else { // Es un archivo
            const directoryPath = path.dirname(fullPath);
    
            if (!fs.existsSync(directoryPath)) {
                fs.mkdirSync(directoryPath, { recursive: true });
            }
    
            console.log(`Creating placeholder for ${path.basename(fullPath)}`);
            try {
                this.createPlaceholderFile(
                    path.basename(fullPath),
                    fileId,
                    0,
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
