import yargs from "yargs";
import z from "zod";

import { drive, logger } from "./drive";

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

const { data } = z.object({ file: z.string() }).safeParse(argv);

if (data) {
  const path = data.file;
  const state = drive.getPlaceholderState(path);
  logger.info({ state });
} else {
  logger.error("Por favor especifica un archivo con --file <path>");
}
