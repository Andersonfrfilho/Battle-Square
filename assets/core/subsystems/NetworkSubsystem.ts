// assets/core/subsystems/NetworkSubsystem.ts
import { EventBus } from "../event-bus/EventBus";

export class NetworkSubsystem {
  private eventBus: EventBus;

  constructor(eventBus: EventBus) {
    this.eventBus = eventBus;
    // Integre com WebSocket/Colyseus para sync
    this.eventBus.on("ACTION_ADDED", ({ pet, action }) => {
      // Envia ação ao servidor (ex: { ownerId: pet.ownerId, action })
      console.log(`Sync ação ${action} para pet ${pet.name}`);
    });
  }
}
