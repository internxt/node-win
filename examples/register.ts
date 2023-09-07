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

function onRenameCallback(newName: string, fileId: string) {
    console.log("Rename callback triggered.");
    console.log("File ID: " + fileId)
    console.log("New name: " + newName)
    return true;
}

drive.connectSyncRoot({
    notifyDeleteCallback: onDeleteCompletionCallback,
    notifyRenameCallback: onRenameCallback
});

/*path, itemId, itemSize(bytes)*/
drive.createItemByPath(`/A (5th copy).pdfs`, '280ab650-acef-4438-8bbc-29863810b24a', 1000);
drive.createItemByPath(`/file1.txt`, 'fa8217c9-2dd6-4641-9180-8206e60368a6', 1000);
drive.createItemByPath(`/only-folder/`, 'fa8217c9-2dd6-4641-9180-8206e60368123', 1000);
drive.createItemByPath(`/folderWithFolder/folder2/`, 'fa8217c9-2dd6-4641-9180-8206e6036845', 1000);
drive.createItemByPath(`/folderWithFile/file2.txt`, 'fa8217c9-2dd6-4641-9180-8206e6036216', 1000);

drive.watchAndWait2(config.syncRootPath);
