import { EnemyComponent } from "../user/enemy/components/EnemyComponent";
import { PlayerComponent } from "../user/player/components/PlayerComponent";

export type GameControllerStartBattleSessionProps = {
  players: PlayerComponent[]; // IDs dos jogadores
  enemies: EnemyComponent[]; // IDs dos inimigos
};
