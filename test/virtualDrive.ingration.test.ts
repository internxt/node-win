import path from 'path';
import fs from 'fs';
import { VirtualDrive } from '../dist/index';
import settings from '../dist/examples/settings';
import { IQueueManager, QueueItem } from '../src';
import { QueueManager } from '../dist/examples/queueManager';
import { generateRandomFilesAndFolders } from '../dist/examples/utils/generate-random-file-tree';
import { createFilesWithSize } from '../dist/examples/utils';
import { ItemsInfoManager } from '../dist/examples/utils';

import {
    onCancelFetchDataCallback,
    onDeleteCallbackWithCallback,
    onFetchDataCallback,
    onFileAddedCallback,
    onMessageCallback,
    onRenameCallbackWithCallback,
} from '../examples/callbacks';

jest.setTimeout(90000);

describe('Integration Test: VirtualDrive + Watcher', () => {
    let drive: VirtualDrive;
    let queueManager: IQueueManager;

    const processedTasks: QueueItem[] = [];

    beforeAll(async () => {
        drive = new VirtualDrive(settings.syncRootPath, settings.defaultLogPath);

        await drive.registerSyncRoot(
            settings.driveName,
            settings.driveVersion,
            '{12345678-1234-1234-1234-123456789012}',
            {
                notifyDeleteCallback: onDeleteCallbackWithCallback,
                notifyRenameCallback: onRenameCallbackWithCallback,
                notifyFileAddedCallback: onFileAddedCallback,
                fetchDataCallback: onFetchDataCallback,
                cancelFetchDataCallback: onCancelFetchDataCallback,
                notifyMessageCallback: onMessageCallback,
            },
            settings.defaultIconPath
        );

        const handlerAdd = async (task: QueueItem) => {
            console.log('[TEST] File added in callback: ' + task.path);
            processedTasks.push(task); // Registramos la tarea procesada
            await new Promise((resolve) => setTimeout(resolve, 500));
            const result = Math.random().toString(36).substring(2, 7);
            await drive.convertToPlaceholder(task.path, result);
            await drive.updateSyncStatus(task.path, task.isFolder, true);
        };

        const handleDehydrate = async (task: QueueItem) => {
            console.log('[TEST] File dehydrated in callback: ' + task.path);
            processedTasks.push(task);
            await new Promise((resolve) => setTimeout(resolve, 500));
            await drive.dehydrateFile(task.path);
        };

        const handleHydrate = async (task: QueueItem) => {
            console.log('[TEST] File hydrated in callback: ' + task.path);
            processedTasks.push(task);
            await new Promise((resolve) => setTimeout(resolve, 500));
            console.log('[TEST] Hydrating file: ' + task.path);
        };

        const handleChangeSize = async (task: QueueItem) => {
            console.log('[TEST] File size changed in callback: ' + task.path);
            processedTasks.push(task);
            await new Promise((resolve) => setTimeout(resolve, 500));
            const result = Math.random().toString(36).substring(2, 7);
            await drive.convertToPlaceholder(task.path, result);
            await drive.updateFileIdentity(task.path, result, false);
            await drive.updateSyncStatus(task.path, task.isFolder, true);
        };

        queueManager = new QueueManager({
            handleAdd: handlerAdd,
            handleHydrate,
            handleDehydrate,
            handleChangeSize,
        });

        await drive.connectSyncRoot();
    });

    afterAll(async () => {
        try {
            await drive.disconnectSyncRoot();
            VirtualDrive.unregisterSyncRoot(settings.syncRootPath);
        } catch (err) {
            console.error('[TEST] Error during teardown:', err);
        }
    });

    it.skip('debe generar archivos y carpetas, luego iniciar el watcher, y disparar eventos al interactuar', async () => {
        const fileGenerationOptions = {
            rootPath: '',
            depth: 1,
            filesPerFolder: 2,
            foldersPerLevel: 2,
            meanSize: 1,
            stdDev: 0.25,
        };


        //@ts-ignore
        const fileMap = await generateRandomFilesAndFolders(drive, fileGenerationOptions);


        createFilesWithSize(settings.syncRootPath, settings.serverRootPath);


        const itemsManager = await ItemsInfoManager.initialize();


        for (const key in fileMap) {
            const value = fileMap[key];
            fileMap[key] = settings.serverRootPath + value.replace('/', '\\');
        }

        await itemsManager.add(fileMap);


        drive.watchAndWait(settings.syncRootPath, queueManager, settings.watcherLogPath);

        console.log('[TEST] Watcher iniciado. Realiza acciones en los archivos.');

        const newFileName = 'my_new_test_file.txt';
        const newFilePath = path.join(settings.syncRootPath, newFileName);

        console.log("newFilePath: ", newFilePath);

        fs.writeFileSync(newFilePath, 'Hello from Integration Test (new file)', 'utf8');


        await new Promise((resolve) => setTimeout(resolve, 80000));

        expect(processedTasks.length).toBeGreaterThanOrEqual(1);

        console.log('[TEST] Se detectó el cambio. Se finaliza el test de integración.');
    });
});