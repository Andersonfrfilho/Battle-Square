import pino from "pino";
import { LoggerInterface } from "./interface";

const logger = pino({
  level: __DEV__ ? "debug" : "info", // debug no dev, info+ em produção
  transport: __DEV__
    ? { target: "pino-pretty", options: { colorize: true } }
    : undefined,
});

export const Logger: LoggerInterface = {
  debug: (msg: string, meta?: any) => logger.debug(meta || {}, msg),
  info: (msg: string, meta?: any) => logger.info(meta || {}, msg),
  warn: (msg: string, meta?: any) => logger.warn(meta || {}, msg),
  error: (msg: string, meta?: any) => logger.error(meta || {}, msg),
};
