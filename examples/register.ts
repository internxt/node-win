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

drive.createItemByPath(`/folder2/subfolder1/fileC.txt`, 'id-C');
drive.createItemByPath(`/folder2/subfolder1/fileD.txt`, 'D');
drive.createItemByPath(`/folder2/subfolder2/fileE.txt`, 'E');

drive.createItemByPath(`/folder3/subfolderA/subsubfolder1/fileF.txt`, 'F');
drive.createItemByPath(`/folder3/subfolderA/subsubfolder2/fileG.txt`, 'G');

drive.createItemByPath(`/folder4/fileH.txt`, 'H');
drive.createItemByPath(`/folder4/subfolderB/fileI.txt`, 'I');
drive.createItemByPath(`/folder4/subfolderB/subsubfolder3/fileJ.txt`, 'J');
drive.createItemByPath(`/folder4/subfolderB/subsubfolder4/fileK.txt`, 'K');
drive.createItemByPath(`/folder4/subfolderC/fileL.txt`, 'L');

drive.createItemByPath(`/folder5/sub1/sub2/sub3/sub4/sub5/fileM.txt`, 'M');
drive.createItemByPath(`/folder5/sub1/fileN.txt`, 'N');
drive.createItemByPath(`/folder5/sub1/sub2/fileO.txt`, 'O');

drive.createItemByPath(`/folder6/subX/fileP.txt`, 'P');
drive.createItemByPath(`/folder6/subY/fileQ.txt`, 'Q');
drive.createItemByPath(`/folder6/subZ/fileR.txt`, 'R');

// while(true) {
//     // do nothing
// }

// const combinedFileAttributes = drive.PLACEHOLDER_ATTRIBUTES.FILE_ATTRIBUTE_READONLY;
// const combinenFolderAttributes = drive.PLACEHOLDER_ATTRIBUTES.FOLDER_ATTRIBUTE_READONLY;

// drive.createPlaceholderFile(
//     config.fileName,
//     "742c2caa-eaae-4137-9515-173067c55045",
//     123,
//     combinedFileAttributes,
//     Date.now(),
//     Date.now(),
//     Date.now(),
//     config.syncRootPath
// );

// drive.createPlaceholderDirectory(
//     config.folderName,
//     "542c2caa-eaae-4137-9515-173067c55045",
//     true,
//     0,
//     combinenFolderAttributes,
//     Date.now(),
//     Date.now(),
//     Date.now(),
//     config.syncRootPath
// );

// drive.createPlaceholderFile(
//     config.fileName,
//     "242c2caa-eaae-4137-9515-173067c55045",
//     0,
//     combinenFolderAttributes,
//     Date.now(),
//     Date.now(),
//     Date.now(),
//     `${config.syncRootPath}\\newfolder\\`
// );

// drive.createPlaceholderDirectory(
//     "otherfolder",
//     "542c2caa-eaae-4137-9515-173067c55045",
//     true,
//     0,
//     combinenFolderAttributes,
//     Date.now(),
//     Date.now(),
//     Date.now(),
//     config.syncRootPath
// );

// drive.createPlaceholderDirectory(
//     "insidefolder",
//     "542c2caa-eaae-4137-9515-173067c55045",
//     true,
//     0,
//     combinenFolderAttributes,
//     Date.now(),
//     Date.now(),
//     Date.now(),
//     `${config.syncRootPath}\\otherfolder`
// );


drive.watchAndWait(config.syncRootPath);
