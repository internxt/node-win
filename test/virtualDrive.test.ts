/**
 * virtualDrive.test.ts
 * Este archivo unifica varios tests unitarios para la clase VirtualDrive.
 */

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

  describe('Constructor y creación del syncRootPath', () => {
    it('Debe crear la carpeta syncRootPath si no existe', () => {
      (fs.existsSync as jest.Mock).mockReturnValue(false);

      new VirtualDrive('C:\\test-drive');

      expect(fs.mkdirSync).toHaveBeenCalledWith('C:\\test-drive', { recursive: true });
    });

    it('No crea la carpeta si ya existe', () => {
      (fs.existsSync as jest.Mock).mockReturnValue(true);

      new VirtualDrive('C:\\test-drive');

      expect(fs.mkdirSync).not.toHaveBeenCalled();
    });
  });

  describe('addLoggerPath', () => {
    it('Debe llamar a addon.addLoggerPath con la ruta de logs', () => {
      (fs.existsSync as jest.Mock).mockReturnValue(true);

      new VirtualDrive('C:\\test-drive', 'C:\\mis-logs');
      expect(mockAddon.addLoggerPath).toHaveBeenCalledWith('C:\\mis-logs');
    });
  });

  describe('createFileByPath', () => {
    it('Debe invocar createPlaceholderFile con los argumentos correctos', () => {
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
        'file.txt',               // Nombre del archivo
        'file-id',                // FileId
        1234,                     // Tamaño
        1,                        // FILE_ATTRIBUTE_NORMAL (0x1) u otro valor que uses
        expect.any(String),       // creationTimeStr (en formato Windows)
        expect.any(String),       // lastWriteTimeStr
        expect.any(String),       // lastAccessTimeStr
        expect.stringContaining('C:\\test-drive\\folder\\subfolder') // basePath
      );
    });
  });

  describe('registerSyncRoot', () => {
    it('Debe llamar a addon.registerSyncRoot y asignar callbacks', async () => {
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
        'C:\\test-drive', // ruta
        'MyProvider',     // providerName
        '1.0.0',          // providerVersion
        'provider-id',    // providerId
        'C:\\iconPath'    // logoPath
      );

      // Verificamos que se haya guardado en el drive
      expect(drive.callbacks).toBe(myCallbacks);
    });
  });

  describe('connectSyncRoot', () => {
    it('Debe invocar addon.connectSyncRoot con los callbacks de entrada', async () => {
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
