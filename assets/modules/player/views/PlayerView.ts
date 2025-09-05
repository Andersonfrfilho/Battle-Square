import { _decorator, Animation, Component, Node } from "cc";
import { Player } from "../../enemy/models/Enemy";
import { MAX_HEALTH, MAX_LEVEL, Movements } from "../constants";
const { ccclass, property } = _decorator;

@ccclass("PlayerView")
export class PlayerView extends Component {
  model: Player;
  start() {
    this.model = new Player({
      name: "Player",
      health: MAX_HEALTH,
      level: MAX_LEVEL,
    });
  }

  move(direction: string) {
    // Só lida com a movimentação visual
    const position = this.node.getPosition();
    switch (direction) {
      case Movements.UP:
        this.node.setPosition(position.x, position.y + 10);
        break;
      case Movements.DOWN:
        this.node.setPosition(position.x, position.y - 10);
        break;
    }
  }

  playAttackAnimation() {
    this.node.getComponent(Animation)?.play("attack");
  }

  update(deltaTime: number) {}
}
