import { onCancelFetchDataCallback, onDeleteCallbackWithCallback, onFetchDataCallback, onFileAddedCallback, onMessageCallback, onRenameCallbackWithCallback } from "../examples/callbacks";
import settings from "../examples/settings";
import VirtualDrive from "../src/virtual-drive";

// setupJest.js
jest.mock('../../build/Release/addon.node');

describe('VirtualDrive', () => {
  let drive: VirtualDrive;
  

  beforeEach(() => {
    drive = new VirtualDrive(settings.syncRootPath, settings.defaultLogPath);
    drive.registerSyncRoot(
  settings.driveName,
  settings.driveVersion,
  "{12345678-1234-1234-1234-123456789012}",
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
  });

  afterEach(() => {
  });

  test('debe crear correctamente el directorio root sincronizado', () => {
    // Aquí podrías mockear fs.existsSync y fs.mkdirSync para simular la creación de directorios
    // Jest proporciona jest.mock para interceptar llamadas a módulos
    jest.spyOn(drive, 'createSyncRootFolder');
    drive.createSyncRootFolder();
    expect(drive.createSyncRootFolder).toHaveBeenCalled();
  });

  test('connectSyncRoot debe llamar al addon con los callbacks correctos', async () => {
  const mockAddonConnect = jest.fn().mockResolvedValue('connected');
  jest.mock('../../build/Release/addon.node', () => ({
    connectSyncRoot: mockAddonConnect
  }));

  await drive.connectSyncRoot();
  expect(mockAddonConnect).toHaveBeenCalledWith(expect.anything(), expect.anything());
});

});
