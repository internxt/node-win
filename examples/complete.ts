import VirtualDrive from '../src/virtual-drive';
import * as config from './config.json';
import { promisify } from 'util';

const drive = new VirtualDrive(config.syncRootPath);
const watchAndWaitAsync = promisify(drive.watchAndWait);

async function main() {
    try {
      // Aquí conectamos todo
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
  
      // Luego inicias el monitoreo en una promesa, no bloqueará otros códigos
      await watchAndWaitAsync(config.syncRootPath);
    } catch (err) {
      console.error("An error occurred:", err);
    }
  }
  

  main();

