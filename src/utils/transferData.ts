import fs from "fs";
import { Watcher } from "../watcher/watcher";

function copyFileSync(source: string, destination: string) {
  const data = fs.readFileSync(source);

  fs.writeFileSync(destination, data);

  Watcher.Instance.writeLog(`Copied: ${source} to ${destination}`);
}

export async function transferData(
  sourcePath: string,
  destinationPath: string
) {
  try {
    // Comprobar si el archivo de origen existe
    if (!fs.existsSync(sourcePath)) {
      Watcher.Instance.writeLog(`Source path does not exist: ${sourcePath}`);
      return;
    }

    // Comprobar si la ruta de destino es un directorio
    if (
      fs.existsSync(destinationPath) &&
      fs.statSync(destinationPath).isDirectory()
    ) {
      Watcher.Instance.writeLog(
        `Destination path is a directory, not a file: ${destinationPath}`
      );
      return;
    }

    // Comprobar si el archivo de origen es un archivo
    const stats = fs.statSync(sourcePath);
    if (stats.isFile()) {
      await copyFileSync(sourcePath, destinationPath);
      Watcher.Instance.writeLog(
        `Successfully copied data from ${sourcePath} to ${destinationPath}`
      );
    } else {
      Watcher.Instance.writeLog(`Source path is not a file: ${sourcePath}`);
    }
  } catch (error: any) {
    Watcher.Instance.writeLog(`Error during transfer: ${error.message}`);
  }
}
