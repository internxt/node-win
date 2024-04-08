import VirtualDrive from "../src/virtual-drive";
import settings from "./settings";
import yargs from "yargs";

// Configura yargs
const argv = yargs
  .command("file", "El path del archivo para obtener el estado", {
    path: {
      description: "el path del archivo",
      alias: "f",
      type: "string",
    },
  })
  .help()
  .alias("help", "h").argv;

const drive = new VirtualDrive(settings.syncRootPath, settings.defaultLogPath, {"notifyLogCallback": (message: string, level: number) => {
  console.log(`[EXAMPLE] message: ${message}`);
  console.log("[EXAMPLE] level: " + level);
}});

//@ts-ignore
if (argv.file) {
  //@ts-ignore
  const path = argv.file;
  const state = drive.getPlaceholderState(path);
  console.log(`${path} state:`, state);
  const states = drive.getPlaceholderWithStatePending();
  console.log(`states:`, states);
} else {
  console.log("Por favor especifica un archivo con --file <path>");
}
