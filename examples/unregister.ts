import VirtualDrive from "../src/virtual-drive";
import { deleteInfoItems } from "./info-items-manager";
import settings from "./settings";

VirtualDrive.unregisterSyncRoot(settings.syncRootPath);
deleteInfoItems();
