import VirtualDrive from '../src/virtual-drive';
import * as config from './config.json';

const drive = new VirtualDrive(config.syncRootPath);

drive.registerSyncRoot(
    config.driveName,
    config.driveVersion,
    "{12345678-1234-1234-1234-123456789012}",
);

function onDeleteCompletionCallback(fileId: string) {
    console.log("File ID: " + fileId)
    console.log("Delete completion callback triggered.");
    return true;
}

async function onRenameCallback(newName: string, fileId: string): Promise<boolean> {
    console.log("Rename callback triggered.");
    console.log("File ID: " + fileId);
    console.log("New name: " + newName);

    const a = await (new Promise<boolean>((resolve, reject) => {
        try {

            setTimeout(() => {
                console.log("Inside setTimeout, resolving promise.");
                resolve(true);
            }, 1000)
        } catch (err) {
            console.log("Inside setTimeout, rejecting promise.");
            reject(err);
        }
    }));

    console.log("Promise resolved: ", a);

    return a;
}

function onRenameCallbackWithCallback(newName: string, fileId: string, responseCallback: (response: boolean) => void) {
    console.log("Inside onRenameCallbackWithCallback");
    console.log("typeof responseCallback: ", typeof responseCallback);
    setTimeout(() => responseCallback(true), 1000);
    console.log("Inside onRenameCallbackWithCallback, after setTimeout");
    // onRenameCallback(newName, fileId)
    // .then(result => {
    //     console.log("Inside .then");
    //     console.log("Promise resolved: ", result);
    //     responseCallback(result);
    // }).catch(err => {
    //     console.log("Inside .catch");
    //     console.log("Promise rejected: ", err);
    //     responseCallback(false);
    // });
}

drive.connectSyncRoot({
    notifyDeleteCallback: onDeleteCompletionCallback,
    notifyRenameCallback: onRenameCallbackWithCallback,//onRenameCallback
});

drive.createItemByPath(`/A (5th copy).pdfs`, '280ab650-acef-4438-8bbc-29863810b24a');
drive.createItemByPath(`/folder1/file2.txt`, 'fa8217c9-2dd6-4641-9180-8206e60368a6');

drive.watchAndWait2(config.syncRootPath);
