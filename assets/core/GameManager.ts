// assets/core/GameController.ts
import { _decorator, Component, director } from "cc";
import { Container } from "./di/Container"; // Novo: DI container
import { EventBus } from "./event-bus/EventBus";
import { PetComponent } from "../pet/components/PetComponent"; // Era PetModel
import { ArenaSystem } from "../battle/systems/ArenaSystem"; // Era ArenaController
import { ControlSystem } from "../control/systems/ControlSystem"; // Era ControlController
import { PlayerComponent } from "../user/player/components/PlayerComponent";
import { EnemyComponent } from "../user/enemy/components/EnemyComponent";
import { GameControllerStartBattleSessionProps } from "./types";

const { ccclass } = _decorator;

interface GameManagerProps {
  eventBus?: EventBus;
}

@ccclass("GameManager")
export class GameManager extends Component {
  private static _instance: GameManager;
  private container: Container;
  private eventBus: EventBus;
  players: PlayerComponent[] = [];
  enemies: EnemyComponent[] = [];
  private arena: ArenaSystem;
  private control: ControlSystem;

  constructor(props: GameManagerProps = {}) {
    super();
    this.eventBus = props.eventBus || new EventBus();
    this.container = new Container();
    // Registra subsystems no DI container
    this.container.bind<EventBus>("EventBus").toConstantValue(this.eventBus);
    this.container.bind<ArenaSystem>("ArenaSystem").to(ArenaSystem);
    this.container.bind<ControlSystem>("ControlSystem").to(ControlSystem);
    this.arena = this.container.resolve<ArenaSystem>("ArenaSystem");
    this.control = this.container.resolve<ControlSystem>("ControlSystem");
  }

  public static get instance(): GameManager {
    return this._instance;
  }

  onLoad() {
    if (!GameManager._instance) {
      GameManager._instance = this;
      director.addPersistRootNode(this.node);
    } else {
      this.node.destroy();
    }
  }

  start() {
    console.log("GameManager iniciado.");
    // Dados fictícios para teste
    const playerPets: PetComponent[] = [
      new PetComponent({
        name: "Fluffy",
        type: "Cat",
        attack: 10,
        health: 50,
        defense: 5,
        experience: 0,
        level: 1,
        height: 32,
        width: 32,
        ownerId: "player1",
        position: { x: 0, y: 0 },
        weight: 4,
        speed: 10,
        actions: [], // Inicializa lista de ações
      }),
    ];
    const enemyPets: PetComponent[] = [
      new PetComponent({
        name: "Spike",
        type: "Dog",
        attack: 12,
        health: 45,
        defense: 4,
        experience: 0,
        level: 1,
        height: 32,
        width: 32,
        ownerId: "enemy1",
        position: { x: 2, y: 2 }, // Posição inicial oposta
        weight: 5,
        speed: 8,
        actions: [],
      }),
    ];
    const defaultPlayer = new PlayerComponent({
      name: "Hero",
      health: 100,
      pets: playerPets,
    });
    const defaultEnemy = new EnemyComponent({
      name: "Goblin",
      health: 80,
      pets: enemyPets,
    });
    this.goToArenaScene({
      players: [defaultPlayer],
      enemies: [defaultEnemy],
    });
  }

  goToArenaScene({ players, enemies }: GameControllerStartBattleSessionProps) {
    this.players = players;
    this.enemies = enemies;
    director.loadScene("Arena"); // Carrega cena Arena
    this.eventBus.emit("GAME_STARTED", {
      log: "Jogo iniciado, carregando arena.",
    });
  }

  getArena(): ArenaSystem {
    return this.container.resolve<ArenaSystem>("ArenaSystem");
  }

  getControl(): ControlSystem {
    return this.container.resolve<ControlSystem>("ControlSystem");
  }
}
