import { _decorator, Component, Node } from "cc";
import { PlayerController } from "../player/controllers/PlayerController";
import { Player } from "../enemy/models/Enemy";
import { PlayerView } from "../player/views/PlayerView";
const { ccclass, property } = _decorator;

@ccclass("ArenaView")
export class ArenaView extends Component {
  private playerController: PlayerController;
  start() {
    const playerView = this.node
      .getChildByName("Trainer")
      ?.getComponent(PlayerView)!;
    const playerModel = new Player({
      name: "Player",
      health: 100,
      level: 1,
    });
    this.playerController = new PlayerController(playerView, playerModel);

    // Pega o ControlView do Canvas e conecta
    const controls = this.node.scene
      .getChildByName("Canvas")
      ?.getChildByName("Controls");
    controls?.on("control-input", this.onControlInput, this);
  }
  private onControlInput(action: string) {
    this.playerController.handleInput(action);
  }

  update(deltaTime: number) {}
}
