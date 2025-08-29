import { PetComponent } from "../pet/components/PetComponent";

export interface UserComponentProps {
  name: string;
  pets: PetComponent[];
  health?: number;
  maxHp?: number;
  level?: number;
  exp?: number;
  gold?: number;
}
export class UserComponent {
  name: string;
  level: number;
  health: number;
  pets: PetComponent[] = [];
  maxHp: number = 100;
  exp: number = 0;
  gold: number = 0;

  constructor({
    name,
    level,
    health,
    maxHp,
    exp,
    gold,
    pets,
  }: UserComponentProps) {
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
