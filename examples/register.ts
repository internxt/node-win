import VirtualDrive from '../src/virtual-drive';
import * as config from './config.json';
import * as fs from 'fs';

function onDeleteCompletionCallback(fileId: string) {
    console.log("File ID: " + fileId)
    console.log("Delete completion callback triggered.");
}

function onRenameCallback(newName: string, fileId: string) {
    console.log("Rename callback triggered.");
    console.log("File ID: " + fileId)
    console.log("New name: " + newName)
}

const drive = new VirtualDrive(config.syncRootPath);

drive.registerSyncRoot(
    config.driveName,
    config.driveVersion,
    "{12345678-1234-1234-1234-123456789012}",
    {
        notifyDeleteCallback: onDeleteCompletionCallback,
        notifyRenameCallback: onRenameCallback,
        notifyFileAddedCallback: (filePath: string) => {
            console.log("File added callback triggered.");
            console.log("File path: " + filePath)
            
            try {
                const newFilePath = filePath.replace(config.syncRootPath, '');
    
                fs.unlinkSync(filePath);
    
                console.log("Creating placeholder at: " + newFilePath)
                drive.createItemByPath(newFilePath, '280ab650-acef-4438-8bbc-29863810b24a', 10); 
            } catch (error) {
                console.error(error);
            }



        },
    }
);



drive.connectSyncRoot();

drive.createItemByPath(`/A (5th copy).pdfs`, '280ab650-acef-4438-8bbc-29863810b24a', 10); /*path, itemId, itemSize(bytes)*/
drive.createItemByPath(`/folder1/file2.txt`, 'fa8217c9-2dd6-4641-9180-8206e60368a6', 12); /*path, itemId, itemSize(bytes)*/

drive.watchAndWait(config.syncRootPath);
