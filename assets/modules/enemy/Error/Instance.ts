import { GameError } from "../../../shared/error/instance";
import { EnemyErrorParams } from "./types";

export class EnemyError extends GameError {
  constructor({ message, code, details, metadata }: EnemyErrorParams) {
    super({ message, code, details, metadata });
    this.name = "EnemyError";
  }
}
