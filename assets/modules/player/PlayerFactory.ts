import { PlayerModel } from "../player/models/PlayerModel";
import { PetModel } from "../pets/models/PetModel";

export class PlayerFactory {
  static createDefaultPlayer(): PlayerModel {
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

    return new PlayerModel({
      name: "Player",
      exp: 0,
      gold: 100,
      level: 1,
      health: 100,
      maxHp: 100,
      pets: [starterPet],
    });
  }
}
