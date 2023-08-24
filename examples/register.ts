import VirtualDrive from '../src/virtual-drive';
import * as config from './config.json';

const drive = new VirtualDrive(config.syncRootPath);

drive.registerSyncRoot(
    config.driveName,
    config.driveVersion,
    "{12345678-1234-1234-1234-123456789012}",
);

function onDeleteCompletionCallback() {
    console.log("Delete completion callback triggered.");
}

drive.connectSyncRoot( {
    notifyDeleteCompletionCallback: onDeleteCompletionCallback
});

drive.createItemByPath(`/A (5th copy).pdfs`, '1');
drive.createItemByPath(`/folder1/folder2/file2.txt`, '2');

drive.watchAndWait(config.syncRootPath);
