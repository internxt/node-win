import settings from "../examples/settings";
import VirtualDrive from "../src/virtual-drive";

// Mock addon for testing purposes
// const addonMock = {
//   // Mock methods as needed for your tests
//   connectSyncRoot: jest.fn(),
//   createPlaceholderFile: jest.fn(),
//   registerSyncRoot: jest.fn(),
//   addLoggerPath: jest.fn(),
//   // Add more mock methods as per your usage
// };

// // Mock the addon import
// jest.mock(
//   "C:\\Users\\usuario1\\Desktop\\shokworks\\internxt\\node-win\\build\\Release\\addon.node", // Ajusta la ruta según la estructura de tu proyecto
//   () => ({
//     __esModule: true,
//     default: addonMock,
//   })
// );

describe("VirtualDrive", () => {
  let virtualDrive: VirtualDrive;

  beforeEach(() => {
    virtualDrive = new VirtualDrive(
      settings.syncRootPath,
      settings.defaultLogPath
    );
  });

  afterEach(() => {
    jest.clearAllMocks();
    virtualDrive?.disconnectSyncRoot();
  });

  it("should initialize correctly", () => {
    expect(virtualDrive).toBeInstanceOf(VirtualDrive);
  });

  // Ejemplo de una prueba para verificar el llamado a addLoggerPath durante la inicialización
  // it("should call addLoggerPath when initializing with loggerPath", () => {
  //   expect(addonMock.addLoggerPath).toHaveBeenCalledWith(
  //     settings.defaultLogPath
  //   );
  // });

  // it("should create sync root folder if it does not exist", () => {
  //   const fsMock = jest
  //     .spyOn(require("fs"), "existsSync")
  //     .mockReturnValue(false);
  //   const mkdirSyncSpy = jest.spyOn(require("fs"), "mkdirSync");

  //   virtualDrive.createSyncRootFolder();

  //   expect(fsMock).toHaveBeenCalledWith("/test/syncroot");
  //   expect(mkdirSyncSpy).toHaveBeenCalledWith("/test/syncroot", {
  //     recursive: true,
  //   });

  //   fsMock.mockRestore();
  //   mkdirSyncSpy.mockRestore();
  // });

  // Add more tests as needed for your methods
});
