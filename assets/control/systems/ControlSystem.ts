// assets/control/systems/ControlSystem.ts
import { EventBus } from "../../core/event-bus/EventBus";
import { ActionsControls } from "../ControlEnum";
import { GameManager } from "../../core/GameManager";
import { Config } from "../../core/constants/Config";
import { PlayerComponent } from "../../user/player/components/PlayerComponent";
import { EnemyComponent } from "../../user/enemy/components/EnemyComponent";

interface ControlSystemProps {
  eventBus: EventBus;
}

export class ControlSystem {
  private readonly player: PlayerComponent;
  private readonly enemy: EnemyComponent;
  private readonly eventBus: EventBus;
  private petChoiceIndexPlayer: number = 0;
  private petChoiceIndexEnemy: number = 0;

  constructor({ eventBus }: ControlSystemProps) {
    this.player = GameManager.instance.players[0];
    this.enemy = GameManager.instance.enemies[0];
    this.eventBus = eventBus;
  }

  onExecuteClick() {
    this.eventBus.emit("ARENA_USER_EXECUTE_ACTION");
  }

  addActionToActivePet(action: ActionsControls): number {
    const activePet = this.player.pets[this.petChoiceIndexPlayer];
    if (!activePet) return 0;

    if (activePet.actions.length >= Config.MAX_ACTIONS_PER_PET) {
      console.warn("Pet atingiu o limite de ações!");
      return activePet.actions.length;
    }

    activePet.actions.push(action);
    console.log(
      `Ação ${action} adicionada ao pet ${activePet.name}. Ações:`,
      activePet.actions
    );
    this.eventBus.emit("ACTION_ADDED", { pet: activePet, action });
    return activePet.actions.length;
  }
}
