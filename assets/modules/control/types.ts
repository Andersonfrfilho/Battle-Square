import { Node } from "cc";
import { ActionsControls } from "./ControlEnum";

export type AddActionToActivePetProps = ActionsControls;

export type AddButtonListenerProps = {
  buttonNode: Node;
  action: ActionsControls;
};
