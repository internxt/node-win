import VirtualDrive from '../src/virtual-drive';
import settings from './settings';
import { onCancelFetchDataCallback, onDeleteCallbackWithCallback, onFetchDataCallback, onFileAddedCallback, onMessageCallback, onRenameCallbackWithCallback } from './callbacks';
import { createFilesWithSize } from './utils';

const drive = new VirtualDrive(settings.syncRootPath, settings.defaultLogPath);

drive.registerSyncRoot(
    settings.driveName,
    settings.driveVersion,
    "{12345678-1234-1234-1234-123456789012}",
    {
        notifyDeleteCallback: onDeleteCallbackWithCallback,
        notifyRenameCallback: onRenameCallbackWithCallback,
        notifyFileAddedCallback: onFileAddedCallback,
        fetchDataCallback: onFetchDataCallback,
        cancelFetchDataCallback: onCancelFetchDataCallback,
        notifyMessageCallback: onMessageCallback,

    },
    settings.defaultIconPath
)

//print arguments of registerSyncRoot
console.log("settings", settings)

drive.connectSyncRoot();

const fileCreatedAt = Date.now() - 172800000;
const fileUpdatedAt = Date.now() - 86400000;
const folderCreatedAt = Date.now() - 259200000;
const folderUpdatedAt = Date.now() - 345600000;

try {
    // creating files
    drive.createFileByPath(`/A (5th copy).pdfs`, '280ab650-acef-4438-8bbc-29863810b24a', 1000, fileCreatedAt, fileUpdatedAt);
    drive.createFileByPath(`/file1.txt`, 'fa8217c9-2dd6-4641-9180-8206e60368a6', 1000, fileCreatedAt, fileUpdatedAt);
    drive.createFileByPath(`/folderWithFile/file2.txt`, 'fa8217c9-2dd6-4641-9180-8206e6036216', 1000, fileCreatedAt, fileUpdatedAt);
    drive.createFileByPath(`/fakefile.txt`, 'fa8217c9-2dd6-4641-9180-8206e6036843', 57, fileCreatedAt, fileUpdatedAt);
    drive.createFileByPath(`/imagen.rar`, 'fa8217c9-2dd6-4641-9180-8206e60368f1', 80582195, fileCreatedAt, fileUpdatedAt);
    drive.createFileByPath(`/noExtensionFile`, 'fa8217c9-2dd6-4641-9180-8206e5039843', 33020, fileCreatedAt, fileUpdatedAt);

    // creating folders
    drive.createFolderByPath(`/only-folder`, 'fa8217c9-2dd6-4641-9180-8206e60368123', 1000, folderCreatedAt, folderUpdatedAt);
    drive.createFolderByPath(`/F.O.L.D.E.R`, 'fa8217c9-2dd6-4641-9180-8206e80000123', 1000, folderCreatedAt, folderUpdatedAt);
    drive.createFolderByPath(`/folderWithFolder/folder2`, 'fa8217c9-2dd6-4641-9180-8206e6036845', 1000, folderCreatedAt, folderUpdatedAt);
    drive.createFolderByPath(`/folderWithFolder/F.O.L.D.E.R`, 'fa8217c9-2dd6-4641-9180-8206e60400123', 1000, folderCreatedAt, folderUpdatedAt);

    // create items
    drive.createItemByPath(`/item-folder/`, 'fa8217c9-2dd6-4641-9189-8206e60368123', 1000, folderCreatedAt, folderUpdatedAt);
    drive.createItemByPath(`/imagen-item.rar`, 'fa8217c9-2dd6-4641-9180-053fe60368f1', 33020, fileCreatedAt, fileUpdatedAt);

    createFilesWithSize(settings.syncRootPath, settings.serverRootPath)

    drive.watchAndWait(settings.syncRootPath);
} catch (error) {
    drive.disconnectSyncRoot();
    VirtualDrive.unregisterSyncRoot(settings.syncRootPath);
    console.log('[EXAMPLE] error: ' + error);
}


export default drive;