import { GameError } from "db://assets/shared/error/instance";
import { UserModel, UserProps } from "../../user/models/UserModel";
import { ENEMY_ERRORS, ERRORS } from "../Error/constant";
import { EnemyError } from "../Error/Instance";

export class EnemyModel extends UserModel {
  private readonly currentPetIndex: number = 0;
  constructor(param: UserProps) {
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
      throw new GameError(ERRORS.ENEMY_HAS_NO_PETS);
    }

    return this.pets[this.currentPetIndex];
  }
}
