import { GameController } from "../../shared/GameController";
import { PetModel } from "../pets/models/PetModel";
import { PlayerModel } from "../player/models/PlayerModel";
import { ArenaModel } from "./ArenaModel";
import { ArenaView } from "./ArenaView";

interface ArenaControllerParams {
  view: ArenaView;
  model: ArenaModel;
  players: PlayerModel[];
  enemies: PlayerModel[];
}
export class ArenaController {
  private readonly view: ArenaView;
  private readonly model: ArenaModel;
  private readonly players: PlayerModel[];
  private readonly enemies: PlayerModel[];

  constructor({ view, enemies, players, model }: ArenaControllerParams) {
    this.players = GameController.instance.players;
    this.enemies = GameController.instance.enemies;
    this.view = view;
    this.model = new ArenaModel({ blockSize: 64 });
  }

  startBattle() {
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

  processActions(pets: PetModel[]): void {
    for (let i = 0; i < pets.length; i++) {
      for (let j = i + 1; j < pets.length; j++) {
        const petA = pets[i];
        const petB = pets[j];

        if (this.checkCollision(petA, petB)) {
          // colisão -> aplicar ataque/defesa
          petA.applyActions(petA.actions);
          petB.applyActions(petB.actions);
        } else {
          // sem colisão -> cada um aplica ação normal
          petA.applyActions(petA.actions);
          petB.applyActions(petB.actions);
        }
      }
    }
  }
}
