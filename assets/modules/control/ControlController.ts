import { _decorator, Button, Component, Node } from "cc";
import { ControlView } from "./ControlView";
import { ControlModel } from "./ControlModel";
const { ccclass, property } = _decorator;

@ccclass("ControlController")
export class ControlController extends Component {
  private state = new ControlModel();
  private view!: ControlView;

  onLoad() {
    this.view = this.getComponent(ControlView)!;
    this.view.node.on("control-input", this.onControlInput, this);
  }

  private onControlInput(direction: string) {
    switch (direction) {
      case "arrow-up":
        this.state.setDirection(0, 1);
        break;
      case "arrow-down":
        this.state.setDirection(0, -1);
        break;
      case "arrow-left":
        this.state.setDirection(-1, 0);
        break;
      case "arrow-right":
        this.state.setDirection(1, 0);
        break;
      case "stop":
        this.state.setDirection(0, 0);
        break;
    }
  }

  getControlState() {
    return this.state;
  }
}
