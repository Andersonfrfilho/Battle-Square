import { EventBus } from "../../shared/event-bus/EventBus";
import { GameController } from "../../shared/GameController";
import { EnemyModel } from "../enemy/models/EnemyModel";
import { PlayerModel } from "../player/models/PlayerModel";
import { AddActionToActivePetProps } from "./types";

interface ControlControllerProps {
  eventBus?: any;
}
export class ControlController {
  private readonly player: PlayerModel;
  private readonly playerIndex: number = 0;
  private readonly enemy: EnemyModel;
  private readonly enemyIndex: number = 0;
  private petChoiceIndexPlayer: number = 0;
  private petChoiceIndexEnemy: number = 0;
  private eventBus: EventBus;

  constructor({ eventBus }: ControlControllerProps) {
    this.player = GameController.instance.players[this.playerIndex];
    this.enemy = GameController.instance.enemies[this.enemyIndex];
    this.eventBus = eventBus;
  }

  onExecuteClick() {
    this.eventBus.emit("USER_EXECUTE_ACTION");
  }

  // executeTurn() {
  //   console.log("Executando turno da arena...");
  //   this.eventBus.emit("ARENA_ACTION_EXECUTED", { log: "Pet A atacou Pet B" });
  // }

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

    this.arenaController.processSimultaneousActions({
      playerPet: this.player.pets[this.petChoiceIndexPlayer],
      enemyPet: this.enemy.pets[this.petChoiceIndexEnemy],
      actionPlayer: this.player.pets[this.petChoiceIndexPlayer].actions[0],
      actionEnemy: this.enemy.pets[this.petChoiceIndexEnemy].actions[0],
    });
    this.resetActionsPets();
  }

  resetActionsPets() {
    if (!this.player.pets[this.petChoiceIndexPlayer]) return;
    this.player.pets[this.petChoiceIndexPlayer].clearActions();
  }
}
