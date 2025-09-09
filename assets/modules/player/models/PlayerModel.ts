import { Action } from "../../action/ActionModel";
import { PetModel } from "../../pets/models/PetModel";

interface Params {
  name: string;
  pets: PetModel[];
  health?: number;
  maxHp?: number;
  level?: number;
  exp?: number;
  gold?: number;
}

export class PlayerModel {
  name: string;
  pets: PetModel[];
  health: number = 100;
  maxHp: number = 100;
  level: number = 1;
  exp: number = 0;
  gold: number = 0;

  constructor({ name, pets }: Params) {
    this.name = name;
    this.pets = pets;
  }
  actions: Action[] = [];
}
