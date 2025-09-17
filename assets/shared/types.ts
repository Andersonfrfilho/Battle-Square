import { EnemyModel } from "../modules/enemy/models/EnemyModel";
import { PlayerModel } from "../modules/player/models/PlayerModel";

export interface GameControllerStartBattleSessionProps {
  players: PlayerModel[];
  enemies: EnemyModel[];
}
