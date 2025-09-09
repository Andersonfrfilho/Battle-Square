interface UserProps {
  name: string;
  level: number;
  health: number;
}
export class UserModel {
  name: string;
  level: number;
  health: number;

  constructor({ name, level, health }: UserProps) {
    this.name = name;
    this.level = level;
    this.health = health;
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
