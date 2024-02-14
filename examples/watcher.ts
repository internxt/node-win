import VirtualDrive from "../src/virtual-drive";
import { onFileAddedCallback, onMessageCallback } from "./callbacks";
import settings from "./settings";

const drive = new VirtualDrive(settings.syncRootPath, settings.defaultLogPath);

const extraCallbacks = {
    notifyFileAddedCallback: onFileAddedCallback,
    notifyMessageCallback: onMessageCallback,
}
drive.watchAndWaitDetached(settings.syncRootPath, extraCallbacks);