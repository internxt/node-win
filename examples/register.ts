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
}

function onRenameCallback(newName: string, fileId: string) {
    console.log("Rename callback triggered.");
    console.log("File ID: " + fileId)
    console.log("New name: " + newName)
}

drive.connectSyncRoot( {
    notifyDeleteCallback: onDeleteCompletionCallback,
    notifyRenameCallback: onRenameCallback
});

drive.createItemByPath(`/A (5th copy).pdfs`, '280ab650-acef-4438-8bbc-29863810b24a');
drive.createItemByPath(`/folder1/file2.txt`, 'fa8217c9-2dd6-4641-9180-8206e60368a6');

drive.watchAndWait2(config.syncRootPath);
