// GameController.ts
import { _decorator, Component, director } from "cc";
import { PlayerModel } from "../modules/player/models/PlayerModel";
import { EnemyModel } from "../modules/enemy/models/EnemyModel";
import { GameControllerStartBattleSessionProps } from "./types";
import { PetModel } from "../modules/pets/models/PetModel";
const { ccclass } = _decorator;

@ccclass("GameController")
export class GameController extends Component {
  private static _instance: GameController;
  players: PlayerModel[] = [];
  enemies: EnemyModel[] = [];
  public static get instance(): GameController {
    return this._instance;
  }

  onLoad() {
    if (!GameController._instance) {
      GameController._instance = this;
      // Faz com que o node persista entre cenas
      director.addPersistRootNode(this.node);
    } else {
      this.node.destroy();
    }
  }

  goToArenaScene({ players, enemies }: GameControllerStartBattleSessionProps) {
    this.players = players;
    this.enemies = enemies;
    director.loadScene("Arena"); // carrega cena Arena
  }

  start() {
    console.log("GameController iniciado.");
    const playerPets: PetModel[] = [
      new PetModel({
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
      }),
    ];
    const enemyPets: PetModel[] = [
      new PetModel({
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
        position: { x: 0, y: 0 },
        weight: 5,
        speed: 8,
      }),
    ];
    // Cria dados fictícios para player e enemy
    const defaultPlayer = new PlayerModel({
      name: "Hero",
      health: 100,
      pets: playerPets,
    });
    const defaultEnemy = new EnemyModel({
      name: "Goblin",
      health: 80,
      pets: enemyPets,
    });
    console.log("Player:", defaultPlayer);
    console.log("Enemy:", defaultEnemy);
    // Inicia uma sessão de batalha com dados fictícios
    this.goToArenaScene({
      players: [defaultPlayer],
      enemies: [defaultEnemy],
    });
  }
}
