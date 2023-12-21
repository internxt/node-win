import * as fs from 'fs';
import * as path from 'path';
import { promisify } from 'util';
import addon from '../src/virtual-drive';
import * as config from './config.json';

const readdir = promisify(fs.readdir);
const stat = promisify(fs.stat);

const directoryPath = config.syncRootPath;

async function checkSyncStatus(filePath: string): Promise<void> {
    const isSynced = await addon.isFileSynchronizedOrPinned(filePath);
    console.log(`${filePath} está ${isSynced ? 'sincronizado' : 'no sincronizado'}`);
}

async function traverseDirectory(dirPath: string): Promise<void> {
    const files = await readdir(dirPath);
    for (const file of files) {
        const filePath = path.join(dirPath, file);
        const fileStat = await stat(filePath);
        await checkSyncStatus(filePath);

        if (fileStat.isDirectory()) {
            await traverseDirectory(filePath);
        }
    }
}

// Inicia la exploración desde el directorio raíz
traverseDirectory(directoryPath).catch(console.error);