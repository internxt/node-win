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

async function onFetchData(fileId: string): Promise<boolean> {
    console.log("downloading file: " + fileId);
    // simulating a download from a real server
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

async function onFileAddedCallback(filePath: string, callback: (aknowledge : boolean, id: string) => void) {

    try {
        console.log("File added in callback: " + filePath);
        const newFilePath = filePath.replace(config.syncRootPath, '').replace(/\\/g, '/'); //IMPORTANTE CAMBIAR LOS SLASHES
        await new Promise(resolve => setTimeout(() => {
            resolve(undefined);
        }, 1000));

        callback(false, '280ab630-acef-4438-8bbc-29863810b24a'); 
    } catch (error) {
        callback(false, '');
        console.error(error);
    }
}

async function onFetchDataCallback(fileId: string, callback: (data : boolean, path: string) => void ) {
    console.log("file id: " + fileId);
    // simulate a download from a real server and response with the path of the downloaded file of a fake server
    onFetchData(fileId).then((response) => {
        callback(response, "C:\\Users\\gcarl\\Desktop\\fakeserver\\imagen.rar");
    }).catch((err) => {
        callback(false, "C:\\Users\\gcarl\\Desktop\\fakeserver\\imagen.rar");
    });
}

const iconPath = 'C:\\Users\\gcarl\\Downloads\\sicon.ico';
drive.registerSyncRoot(
    config.driveName,
    config.driveVersion,
    "{12345678-1234-1234-1234-123456789012}",
    {
        notifyDeleteCallback: onDeleteCallbackWithCallback,
        notifyRenameCallback: onRenameCallbackWithCallback,
        notifyFileAddedCallback: onFileAddedCallback,
        fetchDataCallback: onFetchDataCallback
    },
    iconPath
)

drive.connectSyncRoot();

drive.createItemByPath(`/A (5th copy).pdfs`, '280ab650-acef-4438-8bbc-29863810b24a', 1000);
drive.createItemByPath(`/file1.txt`, 'fa8217c9-2dd6-4641-9180-8206e60368a6', 1000);
drive.createItemByPath(`/only-folder/`, 'fa8217c9-2dd6-4641-9180-8206e60368123', 1000);
drive.createItemByPath(`/folderWithFolder/folder2/`, 'fa8217c9-2dd6-4641-9180-8206e6036845', 1000);
drive.createItemByPath(`/folderWithFile/file2.txt`, 'fa8217c9-2dd6-4641-9180-8206e6036216', 1000);

drive.createItemByPath(`/fakefile.txt`, 'fa8217c9-2dd6-4641-9180-8206e6036843', 57); // keep in mind that the file size must be the same as the original file
drive.createItemByPath(`/imagen.rar`, 'fa8217c9-2dd6-4641-9180-8206e60368f1', 33020); // keep in mind that the file size must be the same as the original file

drive.watchAndWait(config.syncRootPath);

// disconnect after 10 seconds -> this can use before of unregister
// const timeToWait = 10000;
// setTimeout(
//     () => {
//         console.log("Disconnecting...");
//         drive.disconnectSyncRoot();
//     }
// , timeToWait)