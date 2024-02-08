//@ts-ignore
import VirtualDrive from "../src/virtual-drive";
import settings from "./settings";
import path from "path";
import fs from "fs";

VirtualDrive.unregisterSyncRoot(settings.syncRootPath);

// delete persistence file
const filePath = path.join(__dirname, "filesInfo.json");

fs.promises
  .access(filePath, fs.constants.F_OK)
  .then(() => {
    // El archivo existe, procede a borrarlo
    return fs.promises.unlink(filePath);
  })
  .then(() => {
    console.log("[EXAMPLE] fileInfo.json deleted successfully");
  })
  .catch((error) => {
    if (error.code === "ENOENT") {
      // El archivo no existe, no es necesario borrarlo
      console.log("[EXAMPLE] fileInfo.json does not exist, no need to delete");
    } else {
      // Ocurrió otro error
      console.error("Error:", error);
    }
  });
