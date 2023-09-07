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
class VirtualDrive {
    PLACEHOLDER_ATTRIBUTES: { [key: string]: number };
    syncRootPath: string;
    workerPath: string

    constructor(syncRootPath: string) {

        this.PLACEHOLDER_ATTRIBUTES = {
            FILE_ATTRIBUTE_READONLY: 0x1,
            FILE_ATTRIBUTE_HIDDEN: 0x2,
            FOLDER_ATTRIBUTE_READONLY: 0x1,
        };

        this.syncRootPath = syncRootPath;

        this.workerPath = path.join(__dirname, './watch-worker.js'); //TO DELETE

        this.createSyncRootFolder();
    }
    
    createSyncRootFolder() {
        if (!fs.existsSync(this.syncRootPath)) {
            fs.mkdirSync(this.syncRootPath, { recursive: true });
        }
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

    private isValidFolderPath(path: string) {
        return path.startsWith('/') && path.endsWith('/') && !path.includes('.');
    }
    
    private isValidFilePath(path: string) {
        return path.includes('.');
    }    
    
    async registerSyncRoot(providerName: string, providerVersion: string, providerId: string): Promise<any> {
        return await addon.registerSyncRoot(this.syncRootPath, providerName, providerVersion, providerId);
    }

    unregisterSyncRoot(): any {
        const result = addon.unregisterSyncRoot(this.syncRootPath);
        deleteAllSubfolders(this.syncRootPath);
        return result;
    }

    watchAndWait(path: string): void { // TO DELETE
        const worker = new Worker(this.workerPath, { workerData: { path } });
    
        worker.on('message', (message) => {
          console.log('Mensaje desde el worker:', message);
        });
    
        worker.on('error', (err) => {
          console.error('Error en el worker:', err);
        });
    
        worker.on('exit', (code) => {
            console.log(`Worker stopped with code ${code}`);
          if (code !== 0) {
            console.error(`Worker se detuvo con el código ${code}`);
          }
        });
      }

    watchAndWait2(path: string): void {
        addon.watchAndWait(path);
    }
    
    createItemByPath(relativePath: string, itemId: string, size: number = 0) {
        const fullPath = path.join(this.syncRootPath, relativePath);
        const splitPath = relativePath.split('/').filter(p => p);
        const directoryPath = path.resolve(this.syncRootPath);
        let currentPath = directoryPath;
        if (this.isValidFolderPath(relativePath)) { // Es un directorio
            
            for (const dir of splitPath) {
                if (fs.existsSync(currentPath)) {
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
                            currentPath,
                        );
                    } catch (error) {
                        //@ts-ignore
                        console.error(`Error while creating directory: ${error.message}`);
                    }
                }
                currentPath = path.join(currentPath, dir);
            }
        } else if(this.isValidFilePath(relativePath)) { // Es un archivo
            
            try {
                
                for (let i = 0; i < splitPath.length - 1; i++) { // everything except last element
                    const dir = splitPath[i];
                    if (fs.existsSync(currentPath)) {
                        this.createPlaceholderDirectory(
                            dir,
                            itemId,
                            true,
                            0,
                            this.PLACEHOLDER_ATTRIBUTES.FOLDER_ATTRIBUTE_READONLY,
                            Date.now(),
                            Date.now(),
                            Date.now(),
                            currentPath,
                        );
                    }
                    currentPath = path.join(currentPath, dir);
                }
                // last element is the file                
                this.createPlaceholderFile(
                    path.basename(fullPath),
                    itemId,
                    size,
                    this.PLACEHOLDER_ATTRIBUTES.FILE_ATTRIBUTE_READONLY,
                    Date.now(),
                    Date.now(),
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
    
}

export default VirtualDrive;
