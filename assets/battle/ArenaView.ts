import { _decorator, Component, Node } from "cc";
import { PetComponent } from "../pet/components/PetComponent";
import { ControlSystem } from "../control/systems/ControlSystem";
import { ActionsControls } from "../control/ControlEnum";
import { GameManager } from "../core/GameManager";
import { PlayerComponent } from "../user/player/components/PlayerComponent";
import { PlayerView } from "../user/player/views/PlayerView";
const { ccclass, property } = _decorator;

@ccclass("ArenaView")
export class ArenaView extends Component {
  @property(Node)
  private actionMenu: Node = null; // Prefab com botões de ações

  private controlSystem: ControlSystem;

  start() {
    const playerView = this.node
      .getChildByName("Trainer")
      ?.getComponent(PlayerView);
    const playerModel = new PlayerComponent({
      name: "Player",
      health: 100,
      pets: [],
    });
    this.controlSystem = GameManager.instance.getControl();
    this.initializeCombatUI();
  }

  initializeCombatUI() {
    if (!this.actionMenu) {
      console.warn("ActionMenu não configurado!");
      return;
    }
    // Configura botões de ações
    const buttons = {
      up: this.actionMenu.getChildByName("UpButton"),
      down: this.actionMenu.getChildByName("DownButton"),
      left: this.actionMenu.getChildByName("LeftButton"),
      right: this.actionMenu.getChildByName("RightButton"),
      leftUp: this.actionMenu.getChildByName("LeftUpButton"),
      leftDown: this.actionMenu.getChildByName("LeftDownButton"),
      rightUp: this.actionMenu.getChildByName("RightUpButton"),
      rightDown: this.actionMenu.getChildByName("RightDownButton"),
      attack: this.actionMenu.getChildByName("AttackButton"),
      magic: this.actionMenu.getChildByName("MagicButton"),
      defend: this.actionMenu.getChildByName("DefendButton"),
      dodge: this.actionMenu.getChildByName("DodgeButton"),
      execute: this.actionMenu.getChildByName("ExecuteButton"),
    };

    Object.entries(buttons).forEach(([action, button]) => {
      if (!button) return;
      button.on(Node.EventType.MOUSE_DOWN, () => {
        if (action === "execute") {
          this.controlSystem.onExecuteClick();
        } else {
          this.controlSystem.addActionToActivePet(action as ActionsControls);
        }
      });
    });
  }

  updatePet(pet: PetComponent) {
    console.log(`Atualizando ${pet.name} => HP: ${pet.health}/${pet.health}`);
    // Atualize UI (ex: HealthBar.prefab)
  }

  showResult(message: string) {
    console.log("Resultado:", message);
    // Mostre resultado na UI (ex: Label em HUD.prefab)
  }
}
