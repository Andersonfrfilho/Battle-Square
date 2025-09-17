import { _decorator, Component } from "cc";
import { PlayerController } from "../player/controllers/PlayerController";
import { PlayerView } from "../player/views/PlayerView";
import { PlayerModel } from "../player/models/PlayerModel";
import { PetModel } from "../pets/models/PetModel";
const { ccclass } = _decorator;

@ccclass("ArenaView")
export class ArenaView extends Component {
  private playerController: PlayerController;
  start() {
    const playerView = this.node
      .getChildByName("Trainer")
      ?.getComponent(PlayerView)!;
    const playerModel = new PlayerModel({
      name: "Player",
      pets: [],
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

  updatePet(pet: PetModel) {
    // aqui você pode atualizar barras de vida, sprites, animações etc.
    console.log(`Atualizando ${pet.name} => HP: ${pet.health}/${pet.health}`);

    // Exemplo: se tiver uma barra de vida ligada ao pet
    // const hpPercent = (pet.hp / pet.maxHp) * 100;
    // pet.nodeHealthBar.width = hpPercent;
  }

  showResult(message: string) {
    console.log("Resultado:", message);
    // poderia mostrar uma tela de vitória/empate na UI
  }
}
