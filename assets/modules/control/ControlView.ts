import { _decorator, Button, Component, EventMouse, Node, sys } from "cc";
import { ActionsControls } from "./ControlEnum";
import { BUTTON_NAMES, NUMBER_ACTIONS_ENABLE } from "./constant";
const { ccclass, property } = _decorator;

@ccclass("ControlView")
export class ControlView extends Component {
  actions: ActionsControls[];
  constructor() {
    super();
    this.actions = [];
  }

  onEnable() {
    this.node.children.forEach((child) => {
      if (sys.isMobile) {
        // Apenas eventos de toque em dispositivos móveis
        child.on(
          Node.EventType.TOUCH_START,
          () => this.handleInteraction(child.name),
          this
        );
      } else {
        // Apenas eventos de mouse em desktops
        child.on(
          Node.EventType.MOUSE_DOWN,
          (event: EventMouse) => {
            if (event.getButton() === EventMouse.BUTTON_LEFT) {
              this.handleInteraction(child.name);
            }
          },
          this
        );
      }
      child.on(Node.EventType.TOUCH_END, () => this.onRelease(), this);
      child.on(Node.EventType.TOUCH_CANCEL, () => this.onRelease(), this);
    });
  }

  private handleInteraction(dir: string) {
    // Adiciona o nome ao array, se permitido
    if (this.actions.length < NUMBER_ACTIONS_ENABLE) {
      this.actions.push(dir as ActionsControls);
    }
    // Emite o evento de controle
    this.node.emit("control-input", dir);
    // Atualiza o estado do botão após adicionar ao array
    this.updateButtonState();
  }

  private onPress(dir: string) {
    this.node.emit("control-input", dir);
    this.updateButtonState();
  }

  private onRelease() {
    this.node.emit("control-input", "stop");
  }

  protected update(dt: number): void {}

  private updateButtonState() {
    const buttonStartNode = this.node.getChildByName(BUTTON_NAMES.START);
    console.log("##########", this.actions);
    if (buttonStartNode) {
      if (this.actions.length < NUMBER_ACTIONS_ENABLE) {
        buttonStartNode.active = false; // Oculta o botão
      } else {
        buttonStartNode.active = true; // Exibe o botão
        // const newButton = new Node(BUTTON_NAMES.START);
        // newButton.addComponent(Button);
        // this.node.addChild(newButton);
        // newButton.setPosition(196.008, 9.562);
      }
    }
  }
}
