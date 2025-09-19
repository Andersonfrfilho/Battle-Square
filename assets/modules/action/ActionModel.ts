import { ActionsControls } from "../control/ControlEnum";

interface ActionProps {
  type: ActionsControls;
  power: number;
}
export class Action {
  constructor({ power, type }: ActionProps) {
    this.power = power;
    this.type = type;
  }
  power: number;
  type: ActionsControls;
}
