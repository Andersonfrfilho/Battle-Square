import { _decorator, Component, Node } from "cc";
import { ActionsControls } from "./ControlEnum";
const { ccclass, property } = _decorator;

@ccclass("ControlModel")
export class ControlModel extends Component {
  dirX: number = 0;
  dirY: number = 0;
  actions: Array<ActionsControls>;

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
