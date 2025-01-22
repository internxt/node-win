import { drive } from "./drive";
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
