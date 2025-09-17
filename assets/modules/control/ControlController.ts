import { GameController } from "../../shared/GameController";
import { EnemyModel } from "../enemy/models/EnemyModel";
import { PlayerModel } from "../player/models/PlayerModel";
import { AddActionToActivePetProps } from "./types";

export class ControlController {
  private readonly player: PlayerModel;
  private readonly playerIndex: number = 0;
  private readonly enemy: EnemyModel;
  private readonly enemyIndex: number = 0;
  private petChoiceIndexPlayer: number = 0;
  private petChoiceIndexEnemy: number = 0;

  constructor() {
    this.player = GameController.instance.players[this.playerIndex];
    this.enemy = GameController.instance.enemies[this.enemyIndex];
  }

  addActionToActivePet(action: AddActionToActivePetProps): number {
    if (!this.player.pets[this.petChoiceIndexPlayer]) return;
    const added = this.player.pets[this.petChoiceIndexPlayer].addAction(action);
    if (!added) {
      console.warn("Pet já atingiu o limite de ações!");
    } else {
      console.log(
        "Ação adicionada ao Pet:",
        this.player.pets[this.petChoiceIndexPlayer].name,
        "Ações atuais:",
        this.player.pets[this.petChoiceIndexPlayer].actions
      );
    }
    return this.player.pets[this.petChoiceIndexPlayer].actions.length;
  }

  applyActionsPets() {
    if (!this.player.pets[this.petChoiceIndexPlayer]) return;

    this.player.pets[this.petChoiceIndexPlayer].actions.forEach((action) => {
      this.player.pets[this.petChoiceIndexPlayer].applyAction();
      this.enemy.pets[this.petChoiceIndexEnemy].applyAction();
    });
    this.resetActionsPets();
  }

  resetActionsPets() {
    if (!this.player.pets[this.petChoiceIndexPlayer]) return;
    this.player.pets[this.petChoiceIndexPlayer].clearActions();
  }
}
