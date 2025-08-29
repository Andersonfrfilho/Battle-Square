// assets/enemy/components/EnemyComponent.ts
import { PetComponent } from "../../../pet/components/PetComponent";
import { PlayerComponent } from "../../player/components/PlayerComponent";
import { EnemyError } from "../errors/EnemyErrors";
import { ENEMY_ERRORS } from "../errors/constants";

export interface EnemyProps {
  name: string;
  health: number;
  pets: PetComponent[];
}

export class EnemyComponent extends PlayerComponent {
  private currentPetIndex: number = 0;

  constructor(param: EnemyProps) {
    super(param);
    this.currentPetIndex = 0;
  }

  setActivePet(index: number) {
    if (index >= 0 && index < this.pets.length) {
      this.currentPetIndex = index;
    } else {
      throw new EnemyError(ENEMY_ERRORS.ENEMY_HAS_NO_PETS);
    }
  }

  getActivePet() {
    if (this.pets.length === 0) {
      throw new EnemyError(ENEMY_ERRORS.ENEMY_HAS_NO_PETS);
    }
    return this.pets[this.currentPetIndex];
  }
}
