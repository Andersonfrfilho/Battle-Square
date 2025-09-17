import { _decorator, Component, Node } from "cc";
import { ActionsControls } from "./ControlEnum";
import { ControlController } from "./ControlController";
import { NUMBER_ACTIONS_ENABLE } from "./constant";
import { AddButtonListenerProps } from "./types";
const { ccclass, property } = _decorator;

@ccclass("ControlView")
export class ControlView extends Component {
  private controller: ControlController;

  @property(Node)
  leftButton: Node = null;

  @property(Node)
  leftUpButton: Node = null;

  @property(Node)
  upButton: Node = null;

  @property(Node)
  rightUpButton: Node = null;

  @property(Node)
  rightButton: Node = null;

  @property(Node)
  leftDownButton: Node = null;

  @property(Node)
  downButton: Node = null;

  @property(Node)
  rightDownButton: Node = null;

  @property(Node)
  startRoundButton: Node = null;

  constructor() {
    super();
  }

  protected onLoad(): void {
    this.controller = new ControlController();
    this.addButtonListener({
      buttonNode: this.upButton,
      action: ActionsControls.UP,
    });
    this.addButtonListener({
      buttonNode: this.downButton,
      action: ActionsControls.DOWN,
    });
    this.addButtonListener({
      buttonNode: this.leftButton,
      action: ActionsControls.LEFT,
    });
    this.addButtonListener({
      buttonNode: this.rightButton,
      action: ActionsControls.RIGHT,
    });
    this.addButtonListener({
      buttonNode: this.leftUpButton,
      action: ActionsControls.LEFT_UP,
    });
    this.addButtonListener({
      buttonNode: this.leftDownButton,
      action: ActionsControls.LEFT_DOWN,
    });
    this.addButtonListener({
      buttonNode: this.rightUpButton,
      action: ActionsControls.RIGHT_UP,
    });
    this.addButtonListener({
      buttonNode: this.rightDownButton,
      action: ActionsControls.RIGHT_DOWN,
    });
    this.startRoundButton.on(Node.EventType.MOUSE_DOWN, () =>
      this.onActionClickStartApplyActions()
    );
  }

  private addButtonListener({ buttonNode, action }: AddButtonListenerProps) {
    if (!buttonNode) return;

    buttonNode.on(Node.EventType.MOUSE_DOWN, () => {
      this.onActionClick(action);
    });
  }

  private onActionClickStartApplyActions() {
    this.controller.applyActionsPets();
    this.startRoundButton.active = false;
  }

  private onActionClick(action: ActionsControls) {
    if (this.startRoundButton.active) return;

    const hasAllActions = this.controller.addActionToActivePet(action);
    if (hasAllActions >= NUMBER_ACTIONS_ENABLE) {
      this.startRoundButton.active = true;
    }
  }
}
