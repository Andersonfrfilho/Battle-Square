import { PetComponent } from "db://assets/pet/components/PetComponent";
import { PlayerComponent } from "../components/PlayerComponent";

export class PlayerView {
  private playerComponent: PlayerComponent;

  constructor(playerComponent: PlayerComponent) {
    this.playerComponent = playerComponent;
  }

  updateHealth(health: number) {
    this.playerComponent.health = health;
    // Atualize a UI correspondente
  }

  addPet(pet: PetComponent) {
    this.playerComponent.pets.push(pet);
    // Atualize a UI correspondente
  }
}
