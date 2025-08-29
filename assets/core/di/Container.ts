// shared/di/Container.ts
export type Factory<T> = (c: Container) => T;

export class Container {
  private singletons = new Map<string, any>();
  private factories = new Map<string, Factory<any>>();

  register<T>(key: string, factory: Factory<T>): void {
    this.factories.set(key, factory);
  }

  resolve<T>(key: string): T {
    // já existe um singleton?
    if (this.singletons.has(key)) {
      return this.singletons.get(key);
    }

    // cria sob demanda
    const factory = this.factories.get(key);
    if (!factory) {
      throw new Error(`Factory não encontrada para: ${key}`);
    }

    const instance = factory(this);
    this.singletons.set(key, instance);
    return instance;
  }

  clear(): void {
    this.singletons.clear();
  }
}
