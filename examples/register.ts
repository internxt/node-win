import VirtualDrive from '../src/virtual-drive';
import * as config from './config.json';

const drive = new VirtualDrive();

drive.registerSyncRoot(
    config.syncRootPath,
    config.driveName,
    config.driveVersion,
    "{12345678-1234-1234-1234-123456789012}",
);

drive.connectSyncRoot(config.syncRootPath);

const combinedAttributes = drive.PLACEHOLDER_ATTRIBUTES.FILE_ATTRIBUTE_READONLY;

drive.createPlaceholderFile(
    config.fileName,
    "742c2caa-eaae-4137-9515-173067c55045",
    123456,
    combinedAttributes,
    Date.now(),
    Date.now(),
    Date.now(),
    config.syncRootPath
);

drive.watchAndWait(config.syncRootPath);
