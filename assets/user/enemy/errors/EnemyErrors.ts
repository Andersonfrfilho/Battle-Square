import { GameError } from "../../../core/error/GameError";
import { EnemyErrorParams } from "./types";

export class EnemyError extends GameError {
  constructor({ message, code, details, metadata }: EnemyErrorParams) {
    super({ message, code, details, metadata });
    this.name = "EnemyError";
  }
}
