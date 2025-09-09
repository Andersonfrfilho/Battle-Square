import { ActionsControls } from "./ControlEnum";

export class ControlModel {
  dirX: number = 0;
  dirY: number = 0;
  actions: Array<ActionsControls> = [];

  setDirection(x: number, y: number) {
    this.dirX = x;
    this.dirY = y;
  }

  addActions(actionParam: ActionsControls) {
    this.actions.push(actionParam);
  }

  resetActions() {
    this.actions = [];
  }

  start() {}

  update(deltaTime: number) {}
}
