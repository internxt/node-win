import { resolve } from "path";
import { inspect } from "util";
import winston from "winston";

const { format, transports } = winston;

export const createLogger = (path: string) => {
  return winston.createLogger({
    format: format.errors({ stack: true }),
    transports: [
      new transports.File({
        filename: resolve(path),
        format: format.combine(format.timestamp(), format.json()),
      }),
      new transports.Console({
        format: format.combine(
          format.printf(({ level, message, stack }) => {
            const object: any = { level, message };
            if (stack) object.stack = stack;
            return inspect(object, { colors: true, depth: Infinity, breakLength: Infinity });
          }),
        ),
      }),
    ],
  });
};
