import VirtualDrive from "@/virtual-drive";

import { drive } from "./drive";
import { deleteInfoItems } from "./info-items-manager";

drive.unregisterSyncRoot();
deleteInfoItems();
