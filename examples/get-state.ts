import VirtualDrive from '../src/virtual-drive';
import settings from './settings';

const drive = new VirtualDrive(settings.syncRootPath, settings.defaultLogPath);

const a = drive.getPlaceholderState('\\file1.txt');
const b = drive.getPlaceholderState('\\imagen-item.rar');
console.log("file1.txt", b);
console.log("imagen.rar", a);