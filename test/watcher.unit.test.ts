import chokidar from 'chokidar';
import fs from 'fs';
import { Watcher } from '../src/watcher/watcher';
import {
    PinState,
    SyncState,
    Status,
    Attributes,
} from '../src/types/placeholder.type';

import { IQueueManager, typeQueue } from '../src/queue/queueManager';
import { IVirtualDriveFunctions } from '../src/watcher/watcher.interface';

jest.mock('chokidar', () => ({
    watch: jest.fn().mockReturnValue({
        on: jest.fn().mockReturnThis(),
    }),
}));

jest.mock('fs');

describe('Watcher Tests', () => {
    let instance: Watcher;

    const mockVirtualDriveFn: IVirtualDriveFunctions = {
        CfAddItem: jest.fn(),
        CfHydrate: jest.fn(),
        CfDehydrate: jest.fn(),
        CfNotifyMessage: jest.fn(),
        CfUpdateItem: jest.fn(),
        CfConverToPlaceholder: jest.fn(),
        CfUpdateSyncStatus: jest.fn(),
        CfGetPlaceHolderState: jest.fn(),
        CfGetPlaceHolderIdentity: jest.fn(),
        CfGetPlaceHolderAttributes: jest.fn(),
        UpdatePinState: jest.fn(),
    };

    let mockQueueManager: IQueueManager = {
        //@ts-ignore
        actions: [],
        enqueue: jest.fn(),
    };

    beforeAll(() => {
        instance = Watcher.Instance;
    });

    beforeEach(() => {
        jest.clearAllMocks();
        instance.syncRootPath = 'C:\\test-drive';
        instance.options = {
            ignored: /(^|[\/\\])\../,
            ignoreInitial: true,
        };
        instance.virtualDriveFunctions = mockVirtualDriveFn;
        instance.queueManager = mockQueueManager;
        instance.logPath = 'C:\\test.log';
    });

    describe('watchAndWait', () => {
        it('should configure chokidar with the correct path and options', () => {
            instance.watchAndWait();
            expect(chokidar.watch).toHaveBeenCalledWith('C:\\test-drive', instance.currentOptions);

            const watcher = (chokidar.watch as jest.Mock).mock.results[0].value;
            expect(watcher.on).toHaveBeenCalledWith('add', expect.any(Function));
            expect(watcher.on).toHaveBeenCalledWith('change', expect.any(Function));
            expect(watcher.on).toHaveBeenCalledWith('addDir', expect.any(Function));
            expect(watcher.on).toHaveBeenCalledWith('error', expect.any(Function));
            expect(watcher.on).toHaveBeenCalledWith('raw', expect.any(Function));
            expect(watcher.on).toHaveBeenCalledWith('ready', expect.any(Function));
        });
    });

    describe('onAdd (simulated event)', () => {
        it('should enqueue an "add" task if the file is new', () => {
            //@ts-ignore
            mockVirtualDriveFn.CfGetPlaceHolderIdentity.mockReturnValue(null);
            //@ts-ignore
            mockVirtualDriveFn.CfGetPlaceHolderState.mockReturnValue({
                pinState: PinState.Unspecified,
                syncState: SyncState.NotInSync,
            } as Status);

            const path = 'C:\\test-drive\\folder\\newfile.txt';
            const stats = {
                size: 1024,
                birthtime: new Date(),
                mtime: new Date(),
            };

            (instance as any).onAdd(path, stats);

            expect(mockVirtualDriveFn.CfGetPlaceHolderIdentity).toHaveBeenCalledWith(path);
            expect(mockVirtualDriveFn.CfGetPlaceHolderState).toHaveBeenCalledWith(path);
            expect(mockQueueManager.enqueue).toHaveBeenCalledWith({
                path,
                type: typeQueue.add,
                isFolder: false,
            });
        });

        it('should not enqueue if the file is already in AlwaysLocal and InSync states', () => {
            //@ts-ignore
            mockVirtualDriveFn.CfGetPlaceHolderIdentity.mockReturnValue('existing-file-id');
            //@ts-ignore
            mockVirtualDriveFn.CfGetPlaceHolderState.mockReturnValue({
                pinState: PinState.AlwaysLocal,
                syncState: SyncState.InSync,
            } as Status);

            const path = 'C:\\test-drive\\folder\\existingFile.txt';
            const stats = {
                size: 2048,
                birthtime: new Date(),
                mtime: new Date(),
            };

            (instance as any).onAdd(path, stats);

            expect(mockQueueManager.enqueue).not.toHaveBeenCalled();
        });
    });
});
