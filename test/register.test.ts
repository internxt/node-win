import settings from "../examples/settings";
import VirtualDrive from "../src/virtual-drive";


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


});
