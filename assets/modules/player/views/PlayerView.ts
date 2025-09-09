import { _decorator, Animation, Component, Node } from "cc";
import { MAX_HEALTH, MAX_LEVEL, Movements } from "../constants";
import { PlayerModel } from "../models/PlayerModel";
const { ccclass, property } = _decorator;

@ccclass("PlayerView")
export class PlayerView extends Component {
  model: PlayerModel;
  start() {
    this.model = new PlayerModel({
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
