import { PetModel } from "../pets/models/PetModel";

export interface Action {
  type: "attack" | "defend" | "heal"; // pode expandir
  power: number;
}

export class ArenaModel {
  player1Actions: Action[] = [];
  player2Actions: Action[] = [];
  round: number = 1;

  constructor(public player1Pet: PetModel, public player2Pet: PetModel) {}

  setPlayerActions(player: 1 | 2, actions: Action[]) {
    if (player === 1) this.player1Actions = actions;
    else this.player2Actions = actions;
  }
}
