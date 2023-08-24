import VirtualDrive from '../src/virtual-drive';
import * as config from './config.json';

const drive = new VirtualDrive(config.syncRootPath);

drive.connectSyncRoot({
    notifyDeleteCompletionCallback: () => {
        console.log("notifyDeleteCompletionCallback");
    }
});
