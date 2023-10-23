//@ts-ignore
import addon from '../../build/Release/addon';
import VirtualDrive from '../src/virtual-drive';

import * as config from './config.json';

VirtualDrive.unregisterSyncRoot(config.syncRootPath);
// cloud items to sync
// should customize this to your own cloud storage test folder
const cloudItems: string[] =  [
    "C:\\Users\\User\\Desktop\\carpeta\\ab",
    "C:\\Users\\User\\Desktop\\carpeta\\file1.txt",
    "C:\\Users\\User\\Desktop\\carpeta\\folderWithFile",
    "C:\\Users\\User\\Desktop\\carpeta\\folderWithFile\\file2.txt",
    "C:\\Users\\User\\Desktop\\carpeta\\A",
    "C:\\Users\\User\\Desktop\\carpeta\\A\\ab - Copy",
    "C:\\Users\\User\\Desktop\\carpeta\\A\\ab - Copy - Copy",
    "C:\\Users\\User\\Desktop\\carpeta\\A\\B",
    "C:\\Users\\User\\Desktop\\carpeta\\A\\B\\ab - Copy - Copy",
    "C:\\Users\\User\\Desktop\\carpeta\\A\\B\\C",
  ];
VirtualDrive.syncAndCleanUp(config.syncRootPath, cloudItems);