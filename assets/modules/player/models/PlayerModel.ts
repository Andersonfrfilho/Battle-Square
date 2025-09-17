import { UserModel, UserProps } from "../../user/models/UserModel";

export class PlayerModel extends UserModel {
  constructor(param: UserProps) {
    super(param);
  }
}
