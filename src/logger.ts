import pino from "pino";
import { cwd } from "process";

const transport = pino.transport({
  targets: [
    {
      target: "pino/file",
      options: { destination: `${cwd()}/node-win.log` },
    },
    {
      target: "pino-pretty",
      options: {
        colorize: true,
        singleLine: true,
      },
    },
  ],
});

export const logger = pino({}, transport);
