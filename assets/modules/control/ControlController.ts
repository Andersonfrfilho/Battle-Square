import { ControlModel } from "./ControlModel";
import { ControlView } from "./ControlView";

export class ControlController {
  private model = new ControlModel();
  private view!: ControlView;

  onLoad() {
    this.view = this.getComponent(ControlView)!;
    this.view.node.on("control-input", this.onControlInput, this);
  }

  private onControlInput(direction: string) {
    switch (direction) {
      case "arrow-up":
        this.model.setDirection(0, 1);
        break;
      case "arrow-down":
        this.model.setDirection(0, -1);
        break;
      case "arrow-left":
        this.model.setDirection(-1, 0);
        break;
      case "arrow-right":
        this.model.setDirection(1, 0);
        break;
      case "stop":
        this.model.setDirection(0, 0);
        break;
    }
  }

  getControlState() {
    return this.model;
  }
}
