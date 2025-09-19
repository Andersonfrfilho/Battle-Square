import { EVENT } from "./type";

// EventBus.ts
type EventBusOnParams = {
  event: EVENT;
  callback: <T>(payload?: T | unknown) => void | Promise<void>;
};

type EventBusEmitParams = {
  event: EVENT;
  payload?: any;
};
export class EventBus {
  private events: Map<string, EventBusOnParams[]> = new Map();

  on({ event, callback }: EventBusOnParams) {
    if (!this.events.has(event)) this.events.set(event, []);

    this.events.get(event)!.push({ event, callback });
  }

  emit({ event, payload }: EventBusEmitParams) {
    if (this.events.has(event)) {
      this.events.get(event)!.forEach(({ callback }) => callback(payload));
    }
  }
}

export const eventBus = new EventBus();
