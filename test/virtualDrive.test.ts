jest.mock('../build/Release/addon.node');
jest.mock('fs');

import fs from 'fs';
import { VirtualDrive } from '../dist/index';
export type NapiCallbackFunction = (...args: any[]) => any;

export type InputSyncCallbacks = {
  fetchDataCallback?: NapiCallbackFunction;
  validateDataCallback?: NapiCallbackFunction;
  cancelFetchDataCallback?: NapiCallbackFunction;
  fetchPlaceholdersCallback?: NapiCallbackFunction;
  cancelFetchPlaceholdersCallback?: NapiCallbackFunction;
  notifyFileOpenCompletionCallback?: NapiCallbackFunction;
  notifyFileCloseCompletionCallback?: NapiCallbackFunction;
  notifyDehydrateCallback?: NapiCallbackFunction;
  notifyDehydrateCompletionCallback?: NapiCallbackFunction;
  notifyDeleteCallback?: NapiCallbackFunction;
  notifyDeleteCompletionCallback?: NapiCallbackFunction;
  notifyRenameCallback?: NapiCallbackFunction;
  notifyRenameCompletionCallback?: NapiCallbackFunction;
  noneCallback?: NapiCallbackFunction;
};

export type ExtraCallbacks = {
  notifyFileAddedCallback?: NapiCallbackFunction;
  notifyMessageCallback?: NapiCallbackFunction;
};

type Callbacks = InputSyncCallbacks & ExtraCallbacks;

const mockAddon = require('../build/Release/addon.node');

describe('VirtualDrive Tests', () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  describe('Constructor and syncRootPath creation', () => {
    it('should create the syncRootPath folder if it does not exist', () => {
      (fs.existsSync as jest.Mock).mockReturnValue(false);

      new VirtualDrive('C:\\test-drive');

      expect(fs.mkdirSync).toHaveBeenCalledWith('C:\\test-drive', { recursive: true });
    });

    it('should not create the folder if it already exists', () => {
      (fs.existsSync as jest.Mock).mockReturnValue(true);

      new VirtualDrive('C:\\test-drive');

      expect(fs.mkdirSync).not.toHaveBeenCalled();
    });
  });

  describe('addLoggerPath', () => {
    it('should call addon.addLoggerPath with the log path', () => {
      (fs.existsSync as jest.Mock).mockReturnValue(true);

      new VirtualDrive('C:\\test-drive', 'C:\\mis-logs');
      expect(mockAddon.addLoggerPath).toHaveBeenCalledWith('C:\\mis-logs');
    });
  });

  describe('createFileByPath', () => {
    it('should invoke createPlaceholderFile with the correct arguments', () => {
      (fs.existsSync as jest.Mock).mockReturnValue(true);

      const drive = new VirtualDrive('C:\\test-drive');
      drive.createFileByPath(
        'folder/subfolder/file.txt',
        'file-id',
        1234,
        1660000000000,
        1660000001000
      );

      expect(mockAddon.createPlaceholderFile).toHaveBeenCalledWith(
        'file.txt',
        'file-id',
        1234,
        1,
        expect.any(String),
        expect.any(String),
        expect.any(String),
        expect.stringContaining('C:\\test-drive\\folder\\subfolder')
      );
    });
  });

  describe('registerSyncRoot', () => {
    it('should call addon.registerSyncRoot and assign callbacks', async () => {
      const drive = new VirtualDrive('C:\\test-drive');
      const myCallbacks: Callbacks = {
        notifyDeleteCallback: jest.fn(),
        notifyRenameCallback: jest.fn(),
      };

      await drive.registerSyncRoot(
        'MyProvider',
        '1.0.0',
        'provider-id',
        myCallbacks,
        'C:\\iconPath'
      );

      expect(mockAddon.registerSyncRoot).toHaveBeenCalledWith(
        'C:\\test-drive',
        'MyProvider',
        '1.0.0',
        'provider-id',
        'C:\\iconPath'
      );

      expect(drive.callbacks).toBe(myCallbacks);
    });
  });

  describe('connectSyncRoot', () => {
    it('should invoke addon.connectSyncRoot with the input callbacks', async () => {
      const drive = new VirtualDrive('C:\\test-drive');
      const myCallbacks: Callbacks = {
        fetchDataCallback: jest.fn(),
      };
      await drive.registerSyncRoot(
        'MyProvider',
        '1.0.0',
        'provider-id',
        myCallbacks,
        'C:\\iconPath'
      );

      await drive.connectSyncRoot();

      expect(mockAddon.connectSyncRoot).toHaveBeenCalledWith(
        'C:\\test-drive',
        drive.getInputSyncCallbacks()
      );
    });
  });
});
