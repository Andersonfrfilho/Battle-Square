import { PetModel } from "../../pets/models/PetModel";

export interface UserProps {
  name: string;
  pets: PetModel[];
  health?: number;
  maxHp?: number;
  level?: number;
  exp?: number;
  gold?: number;
}
export class UserModel {
  name: string;
  level: number;
  health: number;
  pets: PetModel[] = [];
  maxHp: number = 100;
  exp: number = 0;
  gold: number = 0;

  constructor({ name, level, health, maxHp, exp, gold, pets }: UserProps) {
    this.name = name;
    this.level = level;
    this.health = health;
    this.maxHp = maxHp;
    this.exp = exp;
    this.gold = gold;
    this.pets = pets;
  }

  isAlive(): boolean {
    return this.health > 0;
  }

  takeDamage(amount: number): void {
    this.health = Math.max(0, this.health - amount);
  }

  heal(amount: number): void {
    this.health += amount;
  }
}
