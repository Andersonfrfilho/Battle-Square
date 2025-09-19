import { EventBus } from "../../shared/event-bus/EventBus";
import { GameController } from "../../shared/GameController";
import { Action } from "../action/ActionModel";
import { ActionsControls } from "../control/ControlEnum";
import { Actions } from "../control/enums/ActionsEnum";
import { EnemyModel } from "../enemy/models/EnemyModel";
import { PetModel } from "../pets/models/PetModel";
import { PlayerModel } from "../player/models/PlayerModel";
import { Position } from "../position/model/Position";
import { ArenaModel } from "./ArenaModel";
import { ArenaView } from "./ArenaView";

interface ArenaControllerParams {
  view: ArenaView;
  model: ArenaModel;
  players: PlayerModel[];
  enemies: PlayerModel[];
  eventBus: EventBus;
}

interface ExecuteTurnParams {
  indexPlayerSelected: number;
  indexEnemySelected: number;
}

interface ProcessSimultaneousActionsParams {
  playerPet: PetModel;
  enemyPet: PetModel;
  actionPlayer: ActionsControls;
  actionEnemy: ActionsControls;
}

interface ApplyActionParams {
  actor: PetModel;
  target: PetModel;
  action: ActionsControls;
}
export class ArenaController {
  private readonly view: ArenaView;
  private readonly model: ArenaModel;
  private readonly players: PlayerModel[];
  private readonly enemies: EnemyModel[];
  private eventBus: EventBus;

  constructor({
    view,
    enemies,
    players,
    model,
    eventBus,
  }: ArenaControllerParams) {
    this.players = GameController.instance.players;
    this.enemies = GameController.instance.enemies;
    this.view = view;
    this.model = new ArenaModel({ blockSize: 64 });
    this.eventBus.on("USER_EXECUTE_ACTION", this.executeTurn.bind(this));
  }

  executeTurn() {
    // pega fila de ações, ordena por velocidade, resolve resultados
    console.log("Executando turno da arena...");
    this.eventBus.emit("ARENA_ACTION_EXECUTED", { log: "Pet A atacou Pet B" });
  }

  startBattle() {
    this.eventBus.emit("ARENA_TURN_STARTED");
    for (let round = 0; round < 3; round++) {
      this.resolveRound(round);
    }

    this.checkWinner();
  }

  private resolveRound(round: number) {
    const [p1] = this.players;
    const [enemy1] = this.enemies;
    const a1 = p1.actions[round];
    const a2 = enemy1.actions[round];

    // salvar HP inicial
    const hpBeforeP1 = p1.pet.hp;
    const hpBeforeE1 = enemy1.pet.hp;

    // aplicar "em paralelo": cada ação enxerga o HP antes das ações
    const dmgToEnemy = p1.pet.getDamageFromAction(a1, enemy1.pet, hpBeforeE1);
    const dmgToPlayer = enemy1.pet.getDamageFromAction(a2, p1.pet, hpBeforeP1);

    // aplicar os resultados depois
    enemy1.pet.hp = Math.max(0, enemy1.pet.hp - dmgToEnemy);
    p1.pet.hp = Math.max(0, p1.pet.hp - dmgToPlayer);

    // atualizar view
    this.view.updatePet(enemy1.pet);
    this.view.updatePet(p1.pet);
  }

  private checkWinner() {
    const [p1, p2] = this.players;
    if (p1.pet.hp <= 0 && p2.pet.hp <= 0) {
      this.view.showResult("Empate!");
    } else if (p1.pet.hp <= 0) {
      this.view.showResult(`${p2.name} venceu!`);
    } else if (p2.pet.hp <= 0) {
      this.view.showResult(`${p1.name} venceu!`);
    } else {
      this.view.showResult("Ninguém caiu ainda, segue a batalha!");
    }
  }

  checkCollision(petA: PetModel, petB: PetModel): boolean {
    const boundsA = petA.getBounds();
    const boundsB = petB.getBounds();

    return (
      boundsA.x < boundsB.x + boundsB.width &&
      boundsA.x + boundsA.width > boundsB.x &&
      boundsA.y < boundsB.y + boundsB.height &&
      boundsA.y + boundsA.height > boundsB.y
    );
  }

  // executeTurn({
  //   indexPlayerSelected = 0,
  //   indexEnemySelected = 0,
  // }: ExecuteTurnParams) {
  //   console.log("🔄 Iniciando turno...");

  //   const playerPet = this.players[indexPlayerSelected].getActivePet();
  //   const enemyPet = this.enemies[indexEnemySelected].getActivePet();

  //   const petPlayerActions = [...playerPet.actions];
  //   const petEnemyActions = [...enemyPet.actions];

  //   for (
  //     let i = 0;
  //     i < petPlayerActions.length || i < petEnemyActions.length;
  //     i++
  //   ) {
  //     let actionPetPlayer = ActionsControls.NOTHING;
  //     let actionPetEnemy = ActionsControls.NOTHING;
  //     if (petPlayerActions[i]) {
  //       actionPetPlayer = petPlayerActions[i];
  //     }
  //     if (petEnemyActions[i]) {
  //       actionPetEnemy = petEnemyActions[i];
  //     }

  //     this.processSimultaneousActions({
  //       playerPet: playerPet,
  //       enemyPet: enemyPet,
  //       actionPlayer: actionPetPlayer,
  //       actionEnemy: actionPetEnemy,
  //     });
  //   }

  //   // Atualiza HUD
  //   this.view.update(this.player, this.enemy);

  //   // Limpa filas
  //   playerPet.clearActions();
  //   enemyPet.clearActions();
  // }

  private processSimultaneousActions({
    playerPet,
    enemyPet,
    actionPlayer,
    actionEnemy,
  }: ProcessSimultaneousActionsParams) {
    const results: Array<() => void> = [];

    // Player → prepara efeitos
    if (actionPlayer) {
      results.push(() =>
        this.applyAction({
          actor: playerPet,
          target: enemyPet,
          action: actionPlayer,
        })
      );
    }

    // Enemy → prepara efeitos
    if (actionEnemy) {
      results.push(() =>
        this.applyAction({
          actor: enemyPet,
          target: playerPet,
          action: actionEnemy,
        })
      );
    }

    results.forEach((apply) => apply());
  }

  private applyAction({ actor, target, action }: ApplyActionParams) {
    console.log("####### applyAction", action);
    if (action === ActionsControls.DOWN) {
      const newY = actor.position.y - 1;
      const newPosition = new Position(actor.position.x, newY);
      const newPos = this.model.getNextPosition({
        position: newPosition,
        direction: {
          x: 0,
          y: newY,
        },
      });
      if (this.model.isInRange(actor.position, target.position, 1)) {
        console.log(`${actor.name} acertou ${target.name}!`);
        target.hp -= 10; // Exemplo de dano fixo
      }

      if (this.model.isInsideBounds(newPos)) {
        console.log(`${actor.name} moveu para baixo!`);
        actor.position = newPos;
      }
    }

    // if (action.type === "attack") {
    //   if (this.arena.isInRange(actor.position, target.position, action.range)) {
    //     target.health -= action.power;
    //     console.log(`${actor.name} acertou ${target.name}!`);
    //   }
    // }
  }

  // processActions(actions: Action[]) {
  //   const timeline = actions.map((a) => {
  //     const initiative = a.pet.agility + Math.random() * 5;
  //     return { action: a, initiative };
  //   });

  //   timeline.sort((a, b) => b.initiative - a.initiative);

  //   timeline.forEach(({ action }) => {
  //     if (action.type === "ATTACK" && action.target) {
  //       if (this.arena.areClose(action.pet, action.target)) {
  //         action.target.hp -= action.pet.attack;
  //         console.log(`${action.pet.name} atingiu ${action.target.name}`);
  //       }
  //     }

  //     if (action.type === "MOVE" && action.targetPosition) {
  //       this.arena.movePet(action.pet, action.targetPosition);
  //     }
  //   });
  // }
}
