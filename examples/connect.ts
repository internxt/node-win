import VirtualDrive from '../src/virtual-drive';
import * as config from './config.json';

const drive = new VirtualDrive();

drive.connectSyncRoot(config.syncRootPath, {
    notifyDeleteCompletionCallback: () => {
        console.log("notifyDeleteCompletionCallback");
    }
});
