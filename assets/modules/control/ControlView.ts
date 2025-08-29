import { _decorator, Button, Component, Node } from "cc";
const { ccclass, property } = _decorator;

@ccclass("ControlView")
export class ControlView extends Component {
  onEnable() {
    // percorre todos os filhos (ArrowUp, ArrowLeft, etc.)
    this.node.children.forEach((child) => {
      console.log("###########=>", child.name);
      child.on(
        Node.EventType.TOUCH_START,
        () => this.onPress(child.name),
        this
      );
      child.on(Node.EventType.TOUCH_END, () => this.onRelease(), this);
      child.on(Node.EventType.TOUCH_CANCEL, () => this.onRelease(), this);
    });
  }

  private onPress(dir: string) {
    // emite o nome do nó (ex: "ArrowUp", "ArrowLeftDown" etc.)
    this.node.emit("control-input", dir);
  }

  private onRelease() {
    this.node.emit("control-input", "stop");
  }
}
