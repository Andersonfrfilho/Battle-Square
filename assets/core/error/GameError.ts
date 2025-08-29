import { AppErrorParams } from "./types";

export class GameError extends Error {
  private code = 0;
  private details: string | null = null;
  private metadata: any = null;

  constructor({ message, code, details, metadata }: AppErrorParams) {
    super(message);
    this.name = "GameError";
    if (code) this.code = code;
    if (details) this.details = details;
    if (metadata) this.metadata = metadata;
  }
}
