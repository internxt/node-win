import chokidar from 'chokidar';
import fs from 'fs';
import { Watcher } from '../src/watcher/watcher';
import {
  PinState,
  SyncState,
  Status,
  Attributes,
} from '../src/types/placeholder.type';

// Ajusta la ruta de la interfaz si es necesario
import { IQueueManager, typeQueue } from '../src/queue/queueManager';
import { IVirtualDriveFunctions } from '../src/watcher/watcher.interface';

// Mock de chokidar
jest.mock('chokidar', () => ({
  watch: jest.fn().mockReturnValue({
    on: jest.fn().mockReturnThis(),
  }),
}));

// Mock de fs (si lo necesitas)
jest.mock('fs');

describe('Watcher Tests', () => {
  let instance: Watcher;
  
  /**
   * Mock completo de IVirtualDriveFunctions.
   * Ajusta o agrega las funciones que realmente uses en tu código.
   */
  const mockVirtualDriveFn: IVirtualDriveFunctions = {
    // Obligatorio en la interfaz
    CfAddItem: jest.fn(),

    // Opcionales en la interfaz, pero las usas en tu código
    CfHydrate: jest.fn(),
    CfDehydrate: jest.fn(),
    CfNotifyMessage: jest.fn(),
    CfUpdateItem: jest.fn(),
    CfConverToPlaceholder: jest.fn(),

    // Con firma exacta
    CfUpdateSyncStatus: jest.fn(),

    // Funciones para obtener estado/atributos
    CfGetPlaceHolderState: jest.fn(),
    CfGetPlaceHolderIdentity: jest.fn(),
    CfGetPlaceHolderAttributes: jest.fn(),
    UpdatePinState: jest.fn(), // si existe en tu interfaz
  };

  /**
   * Mock de IQueueManager.
   * Ajusta según tu definición real de IQueueManager.
   */
  let mockQueueManager: IQueueManager = {
    // Supongamos que `actions` es obligatorio, lo definimos vacío por ahora
    //@ts-ignore
    actions: [],

    // El método enqueue es el que vemos en tu watcher
    enqueue: jest.fn(),
  };

  beforeAll(() => {
    // Obtenemos la instancia singleton
    instance = Watcher.Instance;
  });

  beforeEach(() => {
    jest.clearAllMocks();

    // Reseteamos las propiedades
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
    it('debe configurar chokidar con la ruta y opciones', () => {
      instance.watchAndWait();
      expect(chokidar.watch).toHaveBeenCalledWith('C:\\test-drive', instance.currentOptions);

      const watcher = (chokidar.watch as jest.Mock).mock.results[0].value;
      // Verificamos que se registren los handlers de eventos
      expect(watcher.on).toHaveBeenCalledWith('add', expect.any(Function));
      expect(watcher.on).toHaveBeenCalledWith('change', expect.any(Function));
      expect(watcher.on).toHaveBeenCalledWith('addDir', expect.any(Function));
      expect(watcher.on).toHaveBeenCalledWith('error', expect.any(Function));
      expect(watcher.on).toHaveBeenCalledWith('raw', expect.any(Function));
      expect(watcher.on).toHaveBeenCalledWith('ready', expect.any(Function));
    });
  });

  describe('onAdd (evento simulado)', () => {
    it('debe encolar una tarea "add" si el archivo es nuevo', () => {
      // Simulamos que CfGetPlaceHolderIdentity retorna null => archivo "nuevo"
      //@ts-ignore
      mockVirtualDriveFn.CfGetPlaceHolderIdentity.mockReturnValue(null);

      // Simulamos un estado con pinState = Unspecified, syncState = NotInSync
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

      // Forzamos la invocación directa de onAdd (privado)
      (instance as any).onAdd(path, stats);

      // Debe llamar CfGetPlaceHolderIdentity y CfGetPlaceHolderState
      expect(mockVirtualDriveFn.CfGetPlaceHolderIdentity).toHaveBeenCalledWith(path);
      expect(mockVirtualDriveFn.CfGetPlaceHolderState).toHaveBeenCalledWith(path);

      // Verificamos la encolada
      expect(mockQueueManager.enqueue).toHaveBeenCalledWith({
        path,
        type: typeQueue.add,
        isFolder: false,
      });
    });

    it('no encola si el archivo ya está en pinState AlwaysLocal y syncState InSync', () => {
      // CfGetPlaceHolderIdentity => "id" => archivo existente
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

      // No debe encolar nada
      expect(mockQueueManager.enqueue).not.toHaveBeenCalled();
    });
  });

  // Agrega más tests para onAddDir, onChange, onRaw, etc. si lo necesitas
});
