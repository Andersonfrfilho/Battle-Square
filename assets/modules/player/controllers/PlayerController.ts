import { _decorator, Component, Node } from "cc";
import { PlayerView } from "../views/PlayerView";
import { Player } from "../../enemy/models/Enemy";
const { ccclass, property } = _decorator;

export class PlayerController {
  constructor(private view: PlayerView, private model: Player) {}
  handleInput(action: string) {
    if (action === "up" || action === "down") {
      this.view.move(action);
    }
    if (action === "attack") {
      this.view.playAttackAnimation();
      // lógica: aplicar dano ao inimigo no Model
    }
  }
}
