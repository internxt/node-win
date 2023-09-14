import VirtualDrive from '../src/virtual-drive';
import * as config from './config.json';
import * as fs from 'fs';

const drive = new VirtualDrive(config.syncRootPath);

async function onDeleteCallback(fileId: string, callback: (response: boolean) => void) {
    console.log("File ID: " + fileId);
    const a = await (new Promise<boolean>((resolve, reject) => {
        try {
            setTimeout(() => {
                resolve(true);
            }, 10)
        } catch (err) {
            reject(err);
        }
    }));

    return a;
}

function onDeleteCallbackWithCallback(fileId: string, callback: (response: boolean) => void) {
    onDeleteCallback(fileId, callback).then((response) => {
        callback(response);
    }).catch((err) => {
        callback(false);
    });
}

async function onRenameCallback(newName: string, fileId: string): Promise<boolean> {
    console.log("File ID: " + fileId);
    console.log("New name: " + newName);

    const a = await (new Promise<boolean>((resolve, reject) => {
        try {

            setTimeout(() => {
                resolve(true);
            }, 1000)
        } catch (err) {
            reject(err);
        }
    }));

    return a;
}

function onRenameCallbackWithCallback(newName: string, fileId: string, responseCallback: (response: boolean) => void) {
    onRenameCallback(newName, fileId).then((response) => {
        responseCallback(response);
    }).catch((err) => {
        responseCallback(false);
    });
}

drive.registerSyncRoot(
    config.driveName,
    config.driveVersion,
    "{12345678-1234-1234-1234-123456789012}",
    {
        notifyDeleteCallback: onDeleteCallbackWithCallback,
        notifyRenameCallback: onRenameCallbackWithCallback,
        notifyFileAddedCallback: async (filePath: string) => {

            try {
                console.log("File added: " + filePath);
                const newFilePath = filePath.replace(config.syncRootPath, '').replace(/\\/g, '/'); //IMPORTANTE CAMBIAR LOS SLASHES
                await new Promise(resolve => setTimeout(() => {
                    resolve(undefined);
                }, 1000));

                fs.unlinkSync(filePath);

                console.log("Creating placeholder at: " + newFilePath)
                drive.createItemByPath(newFilePath.replace('\\','/'), '280ab650-acef-4438-8bbc-29863810b24a', 10); 
                drive.createItemByPath(newFilePath, '280ab651-acef-4438-8bbc-29863810b24a', 10); 
            } catch (error) {
                console.error(error);
            }
        },
    }
)

drive.connectSyncRoot();

drive.createItemByPath(`/A (5th copy).pdfs`, '280ab650-acef-4438-8bbc-29863810b24a', 1000);
drive.createItemByPath(`/file1.txt`, 'fa8217c9-2dd6-4641-9180-8206e60368a6', 1000);
drive.createItemByPath(`/only-folder/`, 'fa8217c9-2dd6-4641-9180-8206e60368123', 1000);
drive.createItemByPath(`/folderWithFolder/folder2/`, 'fa8217c9-2dd6-4641-9180-8206e6036845', 1000);
drive.createItemByPath(`/folderWithFile/file2.txt`, 'fa8217c9-2dd6-4641-9180-8206e6036216', 1000);

drive.watchAndWait(config.syncRootPath);

// disconnect after 10 seconds -> this can use before of unregister
const timeToWait = 10000;
setTimeout(
    () => {
        console.log("Disconnecting...");
        drive.disconnectSyncRoot();
    }
, timeToWait)