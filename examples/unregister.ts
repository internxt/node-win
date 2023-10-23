//@ts-ignore
import addon from '../../build/Release/addon';
import VirtualDrive from '../src/virtual-drive';

import * as config from './config.json';

VirtualDrive.unregisterSyncRoot(config.syncRootPath);
// VirtualDrive.SyncAndCleanUp();