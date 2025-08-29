import { EVENTS_NAMES } from "./type";

// EventBus.ts
type EventBusOnParams = {
  event: EVENTS_NAMES;
  callback: <T>(payload?: T | unknown) => void | Promise<void>;
};

type EventBusEmitParams = {
  event: EVENTS_NAMES;
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
