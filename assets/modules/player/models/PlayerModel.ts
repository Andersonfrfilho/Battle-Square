import { UserModel, UserProps } from "../../user/models/UserModel";

export class PlayerModel extends UserModel {
  private currentPetIndex: number = 0;
  constructor(param: UserProps) {
    super(param);
    this.currentPetIndex = 0;
  }

  getActivePet() {
    return this.pets[this.currentPetIndex];
  }

  setActivePet(index: number) {
    if (index >= 0 && index < this.pets.length) {
      this.currentPetIndex = index;
    } else {
      throw new Error("Invalid pet index");
    }
  }
}
