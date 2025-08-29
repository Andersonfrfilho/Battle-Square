import { ArenaEventsNames } from "../../battle/types";
import { GLOBAL_EVENTS_NAMES } from "./constant";
type GlobalEventsNames = keyof typeof GLOBAL_EVENTS_NAMES;

type Safe<T> = [T] extends [never] ? string : T;
export type EVENTS_NAMES = Safe<GlobalEventsNames> | Safe<ArenaEventsNames>;
