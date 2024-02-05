//@ts-ignore
import VirtualDrive from "../src/virtual-drive";
import settings from "./settings";
import path from "path";
import fs from "fs";

VirtualDrive.unregisterSyncRoot(settings.syncRootPath);

// delete persistence file
const filePath = path.join(__dirname, "fileInfo.json");

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


// delete root sync file content ( IT IS NOT NECESSARY TO DO THIS IN PRODUCTION, IT IS JUST A TEMPORAL BUG FIX)
// https://inxt.atlassian.net/browse/PB-1616?atlOrigin=eyJpIjoiMmFkZjM5MmVhNmYwNDE0MjliOTYwZDU2ODNiZmVkOGQiLCJwIjoiaiJ9

// const folderPath = settings.syncRootPath;

// fs.promises
//   .access(folderPath, fs.constants.F_OK)
//   .then(() => {
//     // La carpeta existe, procede a borrarla
//     return fs.promises.rm(folderPath, { recursive: true });
//   })
//   .then(() => {
//     console.log("[EXAMPLE] Sync root deleted");
//   })
//   .catch((error) => {
//     if (error.code === "ENOENT") {
//       // La carpeta no existe, no es necesario borrarla
//       console.log("La carpeta no existe, no es necesario eliminarla");
//     } else {
//       // Ocurrió otro error
//       console.error("Error:", error);
//     }
//   });

