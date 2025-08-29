// assets/battle/systems/ArenaSystem.ts
import { EventBus } from "../../core/event-bus/EventBus";
import { ArenaComponent } from "../components/ArenaComponent";
import { PlayerComponent } from "../../player/components/PlayerComponent";
import { EnemyComponent } from "../../enemy/components/EnemyComponent";
import { PetComponent } from "../../pet/components/PetComponent";
import { ActionsControls } from "../../control/ControlEnum";
import { GameManager } from "../../core/GameManager";
import { Config } from "../../core/constants/Config";
import { ArenaView } from "../ArenaView";

interface ArenaSystemParams {
  view: ArenaView;
  eventBus: EventBus;
}

interface ProcessSimultaneousActionsParams {
  playerPet: PetComponent;
  enemyPet: PetComponent;
  actionPlayer: ActionsControls;
  actionEnemy: ActionsControls;
}

export class ArenaSystem {
  private readonly view: ArenaView;
  private readonly model: ArenaComponent;
  private readonly players: PlayerComponent[];
  private readonly enemies: EnemyComponent[];
  private readonly eventBus: EventBus;

  constructor({ view, eventBus }: ArenaSystemParams) {
    this.view = view;
    this.model = new ArenaComponent({ blockSize: 64, size: Config.GRID_SIZE });
    this.players = GameManager.instance.players;
    this.enemies = GameManager.instance.enemies;
    this.eventBus = eventBus;
    this.eventBus.on("ARENA_USER_EXECUTE_ACTION", this.executeTurn.bind(this));
    this.eventBus.on("GAME_STARTED", () => this.startBattle());
  }

  startBattle() {
    this.eventBus.emit("ARENA_BATTLE_STARTED", { log: "A batalha começou!" });
    this.view.initializeCombatUI(); // Inicializa UI de combate
  }

  executeTurn() {
    const [player] = this.players;
    const [enemy] = this.enemies;
    const playerPet = player.pets[0]; // Pet ativo (futuro: seleção via UI)
    const enemyPet = enemy.pets[0];

    const playerActions = [...playerPet.actions];
    const enemyActions = [...enemyPet.actions];

    for (
      let i = 0;
      i < Math.max(playerActions.length, enemyActions.length);
      i++
    ) {
      const actionPlayer = playerActions[i] || ActionsControls.NOTHING;
      const actionEnemy = enemyActions[i] || ActionsControls.NOTHING;
      this.processSimultaneousActions({
        playerPet,
        enemyPet,
        actionPlayer,
        actionEnemy,
      });
    }

    this.view.updatePet(playerPet);
    this.view.updatePet(enemyPet);
    playerPet.actions = [];
    enemyPet.actions = [];
    this.eventBus.emit("ARENA_TURN_ENDED", { log: "Turno finalizado" });
    this.checkWinner();
  }

  private processSimultaneousActions({
    playerPet,
    enemyPet,
    actionPlayer,
    actionEnemy,
  }: ProcessSimultaneousActionsParams) {
    const results: Array<() => void> = [];

    if (actionPlayer !== ActionsControls.NOTHING) {
      results.push(() => this.applyAction(playerPet, enemyPet, actionPlayer));
    }
    if (actionEnemy !== ActionsControls.NOTHING) {
      results.push(() => this.applyAction(enemyPet, playerPet, actionEnemy));
    }

    results.forEach((apply) => apply());
  }

  private applyAction(
    actor: PetComponent,
    target: PetComponent,
    action: ActionsControls
  ) {
    if (action === ActionsControls.ATTACK || action === ActionsControls.MAGIC) {
      if (this.model.isInRange(actor.position, target.position, 1)) {
        const damage =
          action === ActionsControls.ATTACK ? actor.attack : actor.attack * 1.5;
        target.health -= damage;
        console.log(
          `${actor.name} usou ${action} e causou ${damage} de dano em ${target.name}!`
        );
      }
    } else if (action === ActionsControls.DEFEND) {
      actor.defense += 5; // Buff temporário
      console.log(`${actor.name} está defendendo!`);
    } else if (action === ActionsControls.DODGE) {
      console.log(`${actor.name} esquivou!`); // Futuro: lógica de esquiva
    } else {
      // Movimentos direcionais
      const dirMap = {
        [ActionsControls.UP]: { x: 0, y: 1 },
        [ActionsControls.DOWN]: { x: 0, y: -1 },
        [ActionsControls.LEFT]: { x: -1, y: 0 },
        [ActionsControls.RIGHT]: { x: 1, y: 0 },
        [ActionsControls.LEFT_UP]: { x: -1, y: 1 },
        [ActionsControls.LEFT_DOWN]: { x: -1, y: -1 },
        [ActionsControls.RIGHT_UP]: { x: 1, y: 1 },
        [ActionsControls.RIGHT_DOWN]: { x: 1, y: -1 },
      };
      const dir = dirMap[action];
      if (dir) {
        const newPos = this.model.getNextPosition({
          position: actor.position,
          direction: dir,
        });
        if (this.model.isInsideBounds(newPos)) {
          actor.position = newPos;
          console.log(`${actor.name} moveu para ${JSON.stringify(newPos)}`);
        }
      }
    }
  }

  private checkWinner() {
    const [player] = this.players;
    const [enemy] = this.enemies;
    if (player.pets[0].health <= 0 && enemy.pets[0].health <= 0) {
      this.view.showResult("Empate!");
    } else if (player.pets[0].health <= 0) {
      this.view.showResult(`${enemy.name} venceu!`);
    } else if (enemy.pets[0].health <= 0) {
      this.view.showResult(`${player.name} venceu!`);
    }
  }
}
