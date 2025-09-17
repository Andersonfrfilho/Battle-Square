import { PlayerModel } from "../player/models/PlayerModel";
import { PetModel } from "../pets/models/PetModel";
import { EnemyModel } from "./models/EnemyModel";

export class EnemyFactory {
  static createDefaultEnemy(): PlayerModel {
    const starterPet = new PetModel({
      name: "Fang",
      type: "Dragon",
      level: 5,
      health: 100,
      attack: 20,
      defense: 15,
      speed: 10,
      experience: 0,
      actions: [],
    });

    return new EnemyModel({
      name: "Enemy",
      exp: 0,
      gold: 100,
      level: 1,
      health: 100,
      maxHp: 100,
      pets: [starterPet],
    });
  }
}
