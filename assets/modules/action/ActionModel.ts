export type ActionType = "ATTACK" | "DEFEND" | "HEAL";

interface ActionProps {
  type: ActionType;
  power: number;
}
export class Action {
  constructor({ power, type }: ActionProps) {
    this.power = power;
    this.type = type;
  }
  power: number;
  type: ActionType;
}
